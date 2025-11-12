#pragma once
#include "GeoImage.hpp"
#include "Util/General.hpp"

namespace RSPIP {

// Single unified API for all image formats
inline std::shared_ptr<GeoImage> GeoImageRead(const std::string &imagePath) {
    GDALAllRegister();
    GDALDataset *dataset =
        static_cast<GDALDataset *>(GDALOpen(imagePath.c_str(), GA_ReadOnly));
    if (!dataset)
        throw std::runtime_error("Failed to open TIFF file: " + imagePath);

    auto img = std::make_shared<GeoImage>();
    img->ImageName = imagePath;
    auto imgWidth = dataset->GetRasterXSize();
    auto imgHeight = dataset->GetRasterYSize();
    auto imgBands = dataset->GetRasterCount();
    img->Projection = dataset->GetProjectionRef();
    img->NonData = cv::Vec3b(0, 0, 0);

    double gt[6] = {0};
    if (dataset->GetGeoTransform(gt) == CE_None)
        img->GeoTransform.assign(gt, gt + 6);

    // 读取所有波段数据
    std::vector<cv::Mat> allBands;
    allBands.reserve(imgBands); // 预分配空间

    for (int bandCount = 1; bandCount <= imgBands; ++bandCount) {
        GDALRasterBand *band = dataset->GetRasterBand(bandCount);
        if (!band) {
            std::cerr << "Error: Band " << bandCount << " is null" << std::endl;
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
            std::cerr << "Error reading band " << bandCount << std::endl;
            continue;
        }

        allBands.push_back(std::move(buffer));
    }

    // 分离基础波段和额外波段
    if (!allBands.empty()) {
        // 取前3个或更少的波段作为BGR数据
        int rgbBandCount = std::min(3, static_cast<int>(allBands.size()));
        std::vector<cv::Mat> rgbBands(allBands.begin(),
                                      allBands.begin() + rgbBandCount);
        reverse(rgbBands.begin(), rgbBands.end()); // BGR顺序
        cv::merge(rgbBands, img->ImageData);

        // 剩余的波段作为额外数据
        if (allBands.size() > 3) {
            img->ExtraBandDatas.assign(allBands.begin() + 3, allBands.end());
        }
    } else {
        std::cerr << "Warning: No bands were successfully read" << std::endl;
    }

    GDALClose(dataset);
    GDALDestroyDriverManager();

    return img;
}

inline std::shared_ptr<Image> NormalImageRead(const std::string &imagePath) {
    cv::Mat img = cv::imread(imagePath, cv::IMREAD_UNCHANGED);
    if (img.empty())
        throw std::runtime_error("Failed to read image: " + imagePath);

    auto imageData = std::make_shared<Image>();
    imageData->ImageData = img;
    imageData->ImageName = imagePath;
    return imageData;
}

inline std::shared_ptr<Image> ImageRead(const std::string &imagePath) {

    if (Util::IsGeoImage(imagePath)) {
        return GeoImageRead(imagePath);
    } else {
        return NormalImageRead(imagePath);
    }

    return nullptr;
}

} // namespace RSPIP
