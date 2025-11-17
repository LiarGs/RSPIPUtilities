#pragma once
#include "Basic/GeoImage.hpp"
#include "Basic/MaskImage.hpp"
#include "Util/Color.hpp"
#include "Util/General.hpp"
using namespace RSPIP;

namespace RSPIP {

std::shared_ptr<Image> NormalImageRead(const std::string &imagePath) {
    cv::Mat img = cv::imread(imagePath, cv::IMREAD_UNCHANGED);
    if (img.empty()) {
        Error("Failed to read image: {}", imagePath);
        throw std::runtime_error("Failed to read image: " + imagePath);
    }

    return std::make_shared<Image>(img, imagePath);
}

std::shared_ptr<GeoImage> GeoImageRead(const std::string &imagePath) {
    GDALAllRegister();
    GDALDataset *dataset = static_cast<GDALDataset *>(GDALOpen(imagePath.c_str(), GA_ReadOnly));
    if (!dataset)
        throw std::runtime_error("Failed to open TIFF file: " + imagePath);

    auto img = std::make_shared<GeoImage>();
    img->ImageName = imagePath;
    auto imgWidth = dataset->GetRasterXSize();
    auto imgHeight = dataset->GetRasterYSize();
    auto imgBands = dataset->GetRasterCount();
    img->Projection = dataset->GetProjectionRef();
    img->NonData = Color::Black;

    double gt[6] = {0};
    if (dataset->GetGeoTransform(gt) == CE_None) {
        img->GeoTransform.assign(gt, gt + 6);
    }

    // 计算图像的边界经纬度
    img->ImageBounds.MinLatitude = gt[3] + imgHeight * gt[5];
    img->ImageBounds.MinLongitude = gt[0];
    img->ImageBounds.MaxLatitude = gt[3];
    img->ImageBounds.MaxLongitude = gt[0] + imgWidth * gt[1];

    // 读取所有波段数据
    std::vector<cv::Mat> allBands;
    allBands.reserve(imgBands); // 预分配空间

    for (int bandCount = 1; bandCount <= imgBands; ++bandCount) {
        GDALRasterBand *band = dataset->GetRasterBand(bandCount);
        if (!band) {
            Error("Error: Band {} is null", bandCount);
            continue;
        }

        // 创建缓冲区
        auto pixelValueType = band->GetRasterDataType();
        cv::Mat buffer(imgHeight, imgWidth,
                       Util::GDALTypeToCVType(pixelValueType));

        // 读取波段数据
        CPLErr err =
            band->RasterIO(GF_Read, 0, 0, imgWidth, imgHeight, buffer.data,
                           imgWidth, imgHeight, pixelValueType, 0, 0);
        if (err != CE_None) {
            Error("Error reading band {}", bandCount);
            continue;
        }

        allBands.push_back(std::move(buffer));
    }

    if (!allBands.empty()) {
        // 取前3个或更少的波段作为BGR数据
        int rgbBandCount = std::min(3, static_cast<int>(allBands.size()));
        std::vector<cv::Mat> rgbBands(allBands.begin(), allBands.begin() + rgbBandCount);
        reverse(rgbBands.begin(), rgbBands.end()); // BGR顺序
        cv::merge(rgbBands, img->ImageData);

        // 剩余的波段作为额外数据
        if (allBands.size() > 3) {
            img->ExtraBandDatas.assign(allBands.begin() + 3, allBands.end());
        }
    } else {
        Error("Warning: No bands were successfully read");
    }

    GDALClose(dataset);
    GDALDestroyDriverManager();

    return img;
}

std::shared_ptr<CloudMask> CloudMaskImageRead(const std::string &imagePath) {

    cv::Mat img = cv::imread(imagePath, cv::IMREAD_UNCHANGED);
    if (img.empty()) {
        Error("Failed to read image: {}", imagePath);
        throw std::runtime_error("Failed to read image: " + imagePath);
    }
    auto cloudMask = std::make_shared<CloudMask>(img, imagePath);

    if (Util::IsGeoImage(imagePath)) {
        GDALAllRegister();
        GDALDataset *dataset = static_cast<GDALDataset *>(GDALOpen(imagePath.c_str(), GA_ReadOnly));
        if (!dataset)
            throw std::runtime_error("Failed to read geoInfo in TIFF file: " + imagePath);

        cloudMask->Projection = dataset->GetProjectionRef();
        double gt[6] = {0};
        if (dataset->GetGeoTransform(gt) == CE_None) {
            cloudMask->GeoTransform.assign(gt, gt + 6);
        }

        // 计算图像的边界经纬度
        cloudMask->ImageBounds.MinLatitude = gt[3] + cloudMask->Height() * gt[5];
        cloudMask->ImageBounds.MinLongitude = gt[0];
        cloudMask->ImageBounds.MaxLatitude = gt[3];
        cloudMask->ImageBounds.MaxLongitude = gt[0] + cloudMask->Width() * gt[1];

        cloudMask->InitCloudMask();
    }

    return cloudMask;
}

std::shared_ptr<Image> ImageRead(const std::string &imagePath, Util::ImageType imageType) {

    if (imageType == Util::ImageType::NormalImage) {
        return NormalImageRead(imagePath);
    } else if (imageType == Util::ImageType::GeoImage) {
        return GeoImageRead(imagePath);
    } else if (imageType == Util::ImageType::CloudMask) {
        return CloudMaskImageRead(imagePath);
    }

    return nullptr;
}

std::shared_ptr<Image> ImageRead(const std::string &imagePath) {

    if (Util::IsGeoImage(imagePath)) {
        return ImageRead(imagePath, Util::ImageType::GeoImage);
    } else {
        return ImageRead(imagePath, Util::ImageType::NormalImage);
    }

    return nullptr;
}

} // namespace RSPIP
