#pragma once
#include "ImageData.hpp"
#include "Util/General.hpp"
#include <filesystem>

namespace RSPIP {

// Single unified API for all image formats
inline std::shared_ptr<ImageData> GeoImageRead(const std::string &imagePath) {
    GDALAllRegister();
    GDALDataset *dataset =
        static_cast<GDALDataset *>(GDALOpen(imagePath.c_str(), GA_ReadOnly));
    if (!dataset)
        throw std::runtime_error("Failed to open TIFF file: " + imagePath);

    auto img = std::make_shared<ImageData>();
    img->ImageName = imagePath;
    auto imgWidth = dataset->GetRasterXSize();
    auto imgHeight = dataset->GetRasterYSize();
    auto imgBands = dataset->GetRasterCount();
    img->Projection = dataset->GetProjectionRef();

    double gt[6] = {0};
    if (dataset->GetGeoTransform(gt) == CE_None)
        img->GeoTransform.assign(gt, gt + 6);

    // Read bands in reverse order to maintain correct channel order(BGRA)
    for (int b = imgBands; b > 0; --b) {
        GDALRasterBand *band = dataset->GetRasterBand(b);
        if (!band)
            continue;

        auto pixelValueType = band->GetRasterDataType();
        cv::Mat buffer(imgHeight, imgWidth, GDALTypeToCVType(pixelValueType));
        CPLErr err =
            band->RasterIO(GF_Read, 0, 0, imgWidth, imgHeight, buffer.data,
                           imgWidth, imgHeight, pixelValueType, 0, 0);
        if (err != CE_None) {
            std::cerr << "Error reading band " << b << std::endl;
            continue;
        }

        img->Data.push_back(buffer);
        img->DataType.push_back(GDALGetDataTypeName(band->GetRasterDataType()));
    }

    GDALClose(dataset);

    return img;
}

inline std::shared_ptr<ImageData>
NormalImageRead(const std::string &imagePath) {
    cv::Mat img = cv::imread(imagePath, cv::IMREAD_UNCHANGED);
    if (img.empty())
        throw std::runtime_error("Failed to read image: " + imagePath);

    auto imageData = std::make_shared<ImageData>();
    imageData->Data.push_back(img);
    imageData->ImageName = imagePath;
    imageData->DataType.push_back("uint8");
    return imageData;
}

inline std::shared_ptr<ImageData> ImageRead(const std::string &imagePath) {
    std::string ext = std::filesystem::path(imagePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".tif" || ext == ".tiff")
        return GeoImageRead(imagePath);
    else if (ext == ".jpg" || ext == ".jpeg" || ext == ".png") {
        return NormalImageRead(imagePath);
    }

    return nullptr;
}

} // namespace RSPIP
