#pragma once
#include "ImageData.hpp"
#include "Util/General.hpp"
#include <filesystem>

namespace RSPIP {

inline bool SaveGeoImage(const std::shared_ptr<ImageData> &image,
                         const std::string &imagePath) {
    GDALAllRegister();
    const char *format = "GTiff";
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(format);
    if (!driver) {
        throw std::runtime_error("GTiff driver not available.");
    }

    int imgWidth = image->Width();
    int imgHeight = image->Height();
    int imgBands = image->GetBandCounts();

    GDALDataset *dataset =
        driver->Create(imagePath.c_str(), imgWidth, imgHeight, imgBands,
                       Util::CVTypeToGDALType(image->GetImageType()), nullptr);
    if (!dataset) {
        throw std::runtime_error("Failed to create TIFF file: " + imagePath);
    }

    if (!image->Projection.empty()) {
        dataset->SetProjection(image->Projection.c_str());
    }
    if (image->GeoTransform.size() == 6) {
        dataset->SetGeoTransform(image->GeoTransform.data());
    }

    // Write bands in reverse order to maintain correct channel order(BGRA)
    for (int b = imgBands; b > 0; --b) {
        GDALRasterBand *band = dataset->GetRasterBand(b);
        if (!band)
            continue;

        CPLErr err = band->RasterIO(
            GF_Write, 0, 0, imgWidth, imgHeight,
            image->BandDatas[imgBands - b].data, imgWidth, imgHeight,
            Util::CVTypeToGDALType(image->GetImageType(imgBands - b)), 0, 0);
        if (err != CE_None) {
            Error("Error writing band {}", b);
            GDALClose(dataset);
            return false;
        }
    }

    GDALClose(dataset);
    Info("Image saved successfully: {}", imagePath);
    return true;
}

inline bool SaveNonGeoImage(const std::shared_ptr<ImageData> &image,
                            const std::string &imagePath) {
    std::filesystem::path pathObj(imagePath);
    std::string extension = pathObj.extension().string();
    const cv::Mat &mergedImg = image->GetMergedData();
    bool success = cv::imwrite(imagePath, mergedImg);
    if (!success) {
        throw std::runtime_error("Failed to write image: " + imagePath);
    } else {
        Info("Image saved successfully: {}", imagePath);
    }
    return success;
}

// Single unified API for all image formats
inline bool SaveImage(const std::shared_ptr<ImageData> &image,
                      const std::string &imagePath) {
    if (Util::IsGeoImage(imagePath)) {
        return SaveGeoImage(image, imagePath);
    } else {
        return SaveNonGeoImage(image, imagePath);
    }
}
} // namespace RSPIP