#include "IO/ImageWriter.h"
#include "Util/General.h"
#include "Util/SuperDebug.hpp"
#include <cmath>
#include <filesystem>
#include <gdal_priv.h>
#include <limits>
#include <numeric>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace RSPIP::IO {

namespace {

std::string _BuildSavePath(const std::string &path, const std::string &name) {
    return (std::filesystem::path(path) / name).string();
}

bool _WriteGeoRaster(const Image &image, const std::string &path, const std::string &name) {
    const std::string savePath = _BuildSavePath(path, name);
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if (driver == nullptr) {
        SuperDebug::Error("GTiff driver not available.");
        throw std::runtime_error("GTiff driver not available.");
    }

    const int imageWidth = image.Width();
    const int imageHeight = image.Height();
    const int imageBands = image.GetBandCounts();

    GDALDataset *dataset = driver->Create(
        savePath.c_str(),
        imageWidth,
        imageHeight,
        imageBands,
        Util::CVTypeToGDALType(image.GetImageType()),
        nullptr);
    if (dataset == nullptr) {
        SuperDebug::Error("Failed to create TIFF file: {}", savePath);
        throw std::runtime_error("Failed to create TIFF file: " + savePath);
    }

    if (image.GeoInfo.has_value()) {
        const auto &geoInfo = *image.GeoInfo;
        if (!geoInfo.Projection.empty()) {
            dataset->SetProjection(geoInfo.Projection.c_str());
        }
        if (geoInfo.GeoTransform.size() == 6) {
            dataset->SetGeoTransform(const_cast<double *>(geoInfo.GeoTransform.data()));
        }
    }

    if (image.HasNoData()) {
        for (int bandIndex = 1; bandIndex <= imageBands; ++bandIndex) {
            const double noDataValue = image.NoDataValues.size() == 1
                                           ? image.NoDataValues.front()
                                           : (bandIndex - 1 < static_cast<int>(image.NoDataValues.size())
                                                  ? image.NoDataValues[static_cast<size_t>(bandIndex - 1)]
                                                  : std::numeric_limits<double>::quiet_NaN());
            if (std::isnan(noDataValue)) {
                continue;
            }
            dataset->GetRasterBand(bandIndex)->SetNoDataValue(noDataValue);
        }
    }

    cv::Mat writeImage;
    if (imageBands == 3) {
        cv::cvtColor(image.ImageData, writeImage, cv::COLOR_BGR2RGB);
    } else if (imageBands == 4) {
        cv::cvtColor(image.ImageData, writeImage, cv::COLOR_BGRA2RGBA);
    } else {
        writeImage = image.ImageData;
    }

    std::vector<int> bandMap(static_cast<size_t>(imageBands));
    std::iota(bandMap.begin(), bandMap.end(), 1);

    const CPLErr error = dataset->RasterIO(
        GF_Write,
        0,
        0,
        imageWidth,
        imageHeight,
        writeImage.data,
        imageWidth,
        imageHeight,
        Util::CVTypeToGDALType(image.GetImageType()),
        imageBands,
        bandMap.data(),
        static_cast<int>(writeImage.elemSize()),
        static_cast<int>(writeImage.step),
        static_cast<int>(writeImage.elemSize1()));

    GDALClose(dataset);
    if (error != CE_None) {
        SuperDebug::Error("Error writing image bands");
        return false;
    }

    SuperDebug::Info("Image saved successfully in: {}", savePath);
    return true;
}

bool _WriteImage(const Image &image, const std::string &path, const std::string &name) {
    if (image.ImageData.empty()) {
        SuperDebug::Error("SaveImage is Empty!");
        return false;
    }

    const std::string savePath = _BuildSavePath(path, name);
    const bool success = cv::imwrite(savePath, image.ImageData);
    if (!success) {
        SuperDebug::Error("Failed to write image: {}", savePath);
    } else {
        SuperDebug::Info("Image saved successfully in: {}", savePath);
    }
    return success;
}

} // namespace

bool SaveImage(const Image &image, const std::string &imagePath, const std::string &imageName) {
    if (image.GeoInfo.has_value()) {
        return _WriteGeoRaster(image, imagePath, imageName);
    }

    return _WriteImage(image, imagePath, imageName);
}

} // namespace RSPIP::IO
