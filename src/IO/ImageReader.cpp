#include "IO/ImageReader.h"
#include "Basic/CloudMask.h"
#include "Basic/GeoImage.h"
#include "Basic/Image.h"
#include "Util/General.h"
#include "Util/SuperDebug.hpp"
#include "gdal_priv.h"
#include <opencv2/imgcodecs.hpp>

namespace RSPIP::IO {

static void _ReadGeoInformation(GeoImage *geoImage, const std::string &imagePath) {
    GDALDataset *dataset = static_cast<GDALDataset *>(GDALOpen(imagePath.c_str(), GA_ReadOnly));

    if (dataset) {
        geoImage->Projection = dataset->GetProjectionRef();
        double gt[6] = {0};
        if (dataset->GetGeoTransform(gt) == CE_None) {
            geoImage->GeoTransform.assign(gt, gt + 6);
        }

        int imgWidth = dataset->GetRasterXSize();
        int imgHeight = dataset->GetRasterYSize();
        // 计算边界
        // 注意：这里假设标准的北方朝上图像。如果含有旋转参数，计算会更复杂
        geoImage->ImageBounds.MinLatitude = gt[3] + imgHeight * gt[5];
        geoImage->ImageBounds.MinLongitude = gt[0];
        geoImage->ImageBounds.MaxLatitude = gt[3];
        geoImage->ImageBounds.MaxLongitude = gt[0] + imgWidth * gt[1];

        GDALClose(dataset);
    } else {
        SuperDebug::Error("Warning: GDAL failed to open file, Geo information might be missing: {}", imagePath);
    }
}

std::unique_ptr<Image> NormalImageRead(const std::string &imagePath) {
    cv::Mat img = cv::imread(imagePath, cv::IMREAD_UNCHANGED);
    if (img.empty()) {
        SuperDebug::Error("Failed to read image: {}", imagePath);
        throw std::runtime_error("Failed to read image: " + imagePath);
        return nullptr;
    }
    return std::make_unique<Image>(img, imagePath);
}

std::unique_ptr<GeoImage> GeoImageRead(const std::string &imagePath) {
    cv::Mat imgData = cv::imread(imagePath, cv::IMREAD_UNCHANGED);
    if (imgData.empty()) {
        SuperDebug::Error("Failed to read image data: {}", imagePath);
        throw std::runtime_error("Failed to read image data: " + imagePath);
        return nullptr;
    }

    auto geoImage = std::make_unique<GeoImage>(imgData, imagePath);
    _ReadGeoInformation(geoImage.get(), imagePath);

    return geoImage;
}

std::unique_ptr<CloudMask> CloudMaskImageRead(const std::string &imagePath) {
    cv::Mat img = cv::imread(imagePath, cv::IMREAD_UNCHANGED);
    if (img.empty()) {
        SuperDebug::Error("Failed to read image: {}", imagePath);
        throw std::runtime_error("Failed to read image: " + imagePath);
    }
    auto cloudMask = std::make_unique<CloudMask>(img, imagePath);

    if (Util::IsGeoImage(imagePath)) {
        _ReadGeoInformation(cloudMask.get(), imagePath);
    }

    return cloudMask;
}

} // namespace RSPIP::IO