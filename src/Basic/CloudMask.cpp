#include "Basic/CloudMask.h"
#include "Interface/IImageVisitor.h"
#include <opencv2/imgproc.hpp>

namespace RSPIP {

CloudMask::CloudMask() : GeoImage() {}
CloudMask::CloudMask(const cv::Mat &imageData) : GeoImage(imageData) {}
CloudMask::CloudMask(const cv::Mat &cloudData, const GeoImage &sourceImage) : GeoImage(cloudData), SourceGeoImage(&sourceImage) {}
CloudMask::CloudMask(const cv::Mat &imageData, const std::string &imageName) : GeoImage(imageData, imageName) {}

void CloudMask::Accept(IImageVisitor &visitor) const {
    visitor.Visit(*this);
}

void CloudMask::Init() {
    if (CloudGroups.empty()) {
        _ExtractCloudGroups();
    }
}

void CloudMask::SetSourceImage(const GeoImage &sourceImage) {
    SourceGeoImage = &sourceImage;
    GeoTransform = sourceImage.GeoTransform;
    Projection = sourceImage.Projection;
}

void CloudMask::_ExtractCloudGroups() {
    cv::Mat labels, stats, centroids;
    int numComponents = cv::connectedComponentsWithStats(ImageData, labels, stats, centroids, 8, CV_32S);

    CloudGroups.resize(numComponents - 1);

    // 提取地理变换参数，避免循环中重复访问
    double gt0 = 0, gt1 = 1, gt2 = 0, gt3 = 0, gt4 = 0, gt5 = 1;
    if (GeoTransform.size() >= 6) {
        gt0 = GeoTransform[0];
        gt1 = GeoTransform[1];
        gt2 = GeoTransform[2];
        gt3 = GeoTransform[3];
        gt4 = GeoTransform[4];
        gt5 = GeoTransform[5];
    }

#pragma omp parallel for
    for (int label = 1; label < numComponents; ++label) {
        auto &cloudGroup = CloudGroups[label - 1];

        // 设置外接矩形
        int x = stats.at<int>(label, cv::CC_STAT_LEFT);
        int y = stats.at<int>(label, cv::CC_STAT_TOP);
        int w = stats.at<int>(label, cv::CC_STAT_WIDTH);
        int h = stats.at<int>(label, cv::CC_STAT_HEIGHT);
        cloudGroup.BoundingBox = cv::Rect(x, y, w, h);

        // 收集该区域内的所有云像素
        std::vector<CloudPixel<unsigned char>> rowPixels;
        rowPixels.reserve(w);
        int pixelNumber = 0;
        for (int row = y; row < y + h; ++row) {
            const int *labelPtr = labels.ptr<int>(row);
            const unsigned char *dataPtr = ImageData.ptr<unsigned char>(row);

            rowPixels.clear();
            for (int col = x; col < x + w; ++col) {
                if (labelPtr[col] == label) {
                    double lat = gt3 + row * gt5 + col * gt4;
                    double lon = gt0 + col * gt1 + row * gt2;
                    rowPixels.emplace_back(dataPtr[col], row, col, lat, lon, pixelNumber++);
                }
            }
            if (!rowPixels.empty()) {
                cloudGroup.CloudPixelMap[row] = std::move(rowPixels);
            }
        }
    }
}

} // namespace RSPIP