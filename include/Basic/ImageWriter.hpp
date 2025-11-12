#pragma once
#include "Basic/GeoImage.hpp"
#include "Util/General.hpp"
#include <filesystem>

namespace RSPIP {

bool SaveGeoImage(const std::shared_ptr<GeoImage> &geoImage,
                  const std::string &imagePath, const std::string &imageName) {
    auto savePath = imagePath + imageName;
    GDALAllRegister();
    const char *format = "GTiff";
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(format);
    if (!driver) {
        throw std::runtime_error("GTiff driver not available.");
    }

    int imgWidth = geoImage->Width();
    int imgHeight = geoImage->Height();
    int imgBands = geoImage->GetBandCounts();

    GDALDataset *dataset = driver->Create(
        savePath.c_str(), imgWidth, imgHeight, imgBands,
        Util::CVTypeToGDALType(geoImage->GetImageType()), nullptr);
    if (!dataset) {
        throw std::runtime_error("Failed to create TIFF file: " + savePath);
    }

    if (!geoImage->Projection.empty()) {
        dataset->SetProjection(geoImage->Projection.c_str());
    }
    if (geoImage->GeoTransform.size() == 6) {
        dataset->SetGeoTransform(geoImage->GeoTransform.data());
    }

    std::vector<cv::Mat> allChannels;
    cv::split(geoImage->ImageData, allChannels);
    // Write bands in reverse order to maintain correct channel order(BGRA)
    reverse(allChannels.begin(), allChannels.end());

    for (const auto &extraBand : geoImage->ExtraBandDatas) {
        allChannels.push_back(extraBand);
    }

    for (int bandCount = 1; bandCount <= imgBands; ++bandCount) {
        GDALRasterBand *band = dataset->GetRasterBand(bandCount);
        if (!band)
            continue;

        CPLErr err = band->RasterIO(
            GF_Write, 0, 0, imgWidth, imgHeight,
            allChannels[bandCount - 1].data, imgWidth, imgHeight,
            Util::CVTypeToGDALType(geoImage->GetImageType()), 0, 0);
        if (err != CE_None) {
            Error("Error writing band {}", bandCount);
            GDALClose(dataset);
            return false;
        }
    }

    GDALClose(dataset);
    GDALDestroyDriverManager();
    Info("GeoImage saved successfully: {}", savePath);
    return true;
}

inline bool SaveNonGeoImage(const std::shared_ptr<Image> &image,
                            const std::string &imagePath,
                            const std::string &imageName) {
    const cv::Mat &mergedImg = image->ImageData;
    bool success = cv::imwrite(imagePath + imageName, mergedImg);
    if (!success) {
        throw std::runtime_error("Failed to write image: " + imagePath);
    } else {
        Info("Image saved successfully: {}", imagePath);
    }
    return success;
}

inline bool SaveImage(const std::shared_ptr<Image> &image,
                      const std::string &imagePath,
                      const std::string &imageName) {
    if (auto geoImage = Util::IsGeoImage(image)) {
        return SaveGeoImage(geoImage, imagePath, imageName);
    } else {
        return SaveNonGeoImage(image, imagePath, imageName);
    }
}
} // namespace RSPIP