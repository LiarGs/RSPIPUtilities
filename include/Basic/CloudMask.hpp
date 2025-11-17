#pragma once
#include "CloudGroup.hpp"
#include "GeoImage.hpp"
#include "Interface/IGeoTransformer.hpp"
#include "MaskImage.hpp"

namespace RSPIP {

class CloudMask : public MaskImage, public IGeoTransformer {

  public:
    CloudMask() : MaskImage() {}

    CloudMask(const cv::Mat &imageData) : MaskImage(imageData) {}

    CloudMask(const cv::Mat &imageData, const std::string &imageName) : MaskImage(imageData, imageName) {}

    void InitCloudMask() {
        _ExtractCloudGroups();
    }

    void SetSourceImage(const std::shared_ptr<GeoImage> &sourceImage) {
        SourceGeoImage = sourceImage;
        GeoTransform = sourceImage->GeoTransform;
        Projection = sourceImage->Projection;
    }

    void PrintImageInfo() override {
        Image::PrintImageInfo();
        IGeoTransformer::PrintGeoInfo();
    }

  private:
    void _ExtractCloudGroups() {
        cv::Mat labels, stats, centroids;
        int num_components = cv::connectedComponentsWithStats(
            ImageData, labels, stats, centroids, 8, CV_32S);

        for (int label = 1; label < num_components; ++label) {
            CloudGroup cloudGroup;

            // 设置外接矩形
            cloudGroup.CloudRect.x = stats.at<int>(label, cv::CC_STAT_LEFT);
            cloudGroup.CloudRect.y = stats.at<int>(label, cv::CC_STAT_TOP);
            cloudGroup.CloudRect.width = stats.at<int>(label, cv::CC_STAT_WIDTH);
            cloudGroup.CloudRect.height = stats.at<int>(label, cv::CC_STAT_HEIGHT);

            // 收集该区域内的所有云像素
            for (int row = cloudGroup.CloudRect.y; row < cloudGroup.CloudRect.y + cloudGroup.CloudRect.height; ++row) {
                for (int col = cloudGroup.CloudRect.x; col < cloudGroup.CloudRect.x + cloudGroup.CloudRect.width; ++col) {
                    if (labels.at<int>(row, col) == label) {
                        auto pixelValue = GetPixelValue<uchar>(row, col);
                        cloudGroup.CloudPixelMap[row].emplace_back(pixelValue, row, col, GetLatitude(row, col), GetLongitude(row, col));
                    }
                }
            }

            CloudGroups.push_back(cloudGroup);
        }
    }

  public:
    std::vector<CloudGroup> CloudGroups;
    std::shared_ptr<GeoImage> SourceGeoImage;
};
} // namespace RSPIP