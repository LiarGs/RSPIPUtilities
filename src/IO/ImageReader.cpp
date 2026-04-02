#include "IO/ImageReader.h"
#include "Util/General.h"
#include "Util/SuperDebug.hpp"
#include "gdal_priv.h"
#include <algorithm>
#include <limits>
#include <numeric>
#include <opencv2/imgcodecs.hpp>

namespace RSPIP::IO {

namespace {

void _ApplyDefaultBlackNoData(Image &image) {
    if (!image.NoDataValues.empty() || image.ImageData.empty()) {
        return;
    }

    const int bandCount = std::max(1, image.GetBandCounts());
    image.NoDataValues.assign(static_cast<size_t>(bandCount), 0.0);
}

std::vector<int> _BuildGeoRasterBandMap(int bandCount) {
    std::vector<int> bandMap(static_cast<size_t>(bandCount));
    std::iota(bandMap.begin(), bandMap.end(), 1);
    if (bandCount == 3) {
        return {3, 2, 1};
    }
    if (bandCount == 4) {
        return {3, 2, 1, 4};
    }
    return bandMap;
}

std::vector<double> _ReadNoDataValues(GDALDataset *dataset) {
    const int bandCount = dataset->GetRasterCount();
    std::vector<double> noDataValues(static_cast<size_t>(std::max(0, bandCount)), std::numeric_limits<double>::quiet_NaN());
    bool hasAnyNoData = false;

    for (int bandIndex = 1; bandIndex <= bandCount; ++bandIndex) {
        int hasNoData = FALSE;
        const double noDataValue = dataset->GetRasterBand(bandIndex)->GetNoDataValue(&hasNoData);
        if (!hasNoData) {
            continue;
        }

        hasAnyNoData = true;
        noDataValues[static_cast<size_t>(bandIndex - 1)] = noDataValue;
    }

    if (!hasAnyNoData) {
        return {};
    }

    return noDataValues;
}

void _TryReadGeoInformation(Image &image, GDALDataset *dataset) {
    if (dataset == nullptr) {
        image.GeoInfo.reset();
        return;
    }

    double geoTransform[6] = {0.0};
    if (dataset->GetGeoTransform(geoTransform) != CE_None) {
        image.GeoInfo.reset();
        return;
    }

    GeoInfo geoInfo = {};
    geoInfo.Projection = dataset->GetProjectionRef();
    geoInfo.GeoTransform.assign(geoTransform, geoTransform + 6);
    geoInfo.RebuildBounds(dataset->GetRasterYSize(), dataset->GetRasterXSize());
    image.GeoInfo = std::move(geoInfo);
}

cv::Mat _ReadGeoRasterPixels(GDALDataset *dataset) {
    const int imageWidth = dataset->GetRasterXSize();
    const int imageHeight = dataset->GetRasterYSize();
    const int bandCount = dataset->GetRasterCount();
    if (bandCount <= 0) {
        throw std::runtime_error("GeoRaster has no raster bands.");
    }

    const GDALDataType bandType = dataset->GetRasterBand(1)->GetRasterDataType();
    for (int bandIndex = 2; bandIndex <= bandCount; ++bandIndex) {
        if (dataset->GetRasterBand(bandIndex)->GetRasterDataType() != bandType) {
            throw std::runtime_error("GeoRaster bands have mixed GDAL data types.");
        }
    }

    const int imageType = Util::GDALTypeToCVType(bandType, bandCount);
    if (imageType < 0) {
        throw std::runtime_error("Unsupported GDAL pixel type.");
    }

    cv::Mat imageData(imageHeight, imageWidth, imageType);
    auto bandMap = _BuildGeoRasterBandMap(bandCount);
    const auto error = dataset->RasterIO(
        GF_Read,
        0,
        0,
        imageWidth,
        imageHeight,
        imageData.data,
        imageWidth,
        imageHeight,
        Util::CVTypeToGDALType(imageType),
        bandCount,
        bandMap.data(),
        static_cast<int>(imageData.elemSize()),
        static_cast<int>(imageData.step),
        static_cast<int>(imageData.elemSize1()));
    if (error != CE_None) {
        throw std::runtime_error("Failed to read GeoRaster pixels.");
    }

    return imageData;
}

Image _ReadGeoRasterImage(const std::string &imagePath) {
    GDALAllRegister();
    GDALDataset *dataset = static_cast<GDALDataset *>(GDALOpen(imagePath.c_str(), GA_ReadOnly));
    if (dataset == nullptr) {
        SuperDebug::Error("Failed to open GeoRaster with GDAL: {}", imagePath);
        throw std::runtime_error("Failed to open GeoRaster: " + imagePath);
    }

    try {
        Image image(_ReadGeoRasterPixels(dataset), imagePath);
        image.NoDataValues = _ReadNoDataValues(dataset);
        _ApplyDefaultBlackNoData(image);
        _TryReadGeoInformation(image, dataset);
        GDALClose(dataset);
        return image;
    } catch (...) {
        GDALClose(dataset);
        throw;
    }
}

} // namespace

Image ReadImage(const std::string &imagePath) {
    if (Util::IsGeoRasterPath(imagePath)) {
        return _ReadGeoRasterImage(imagePath);
    }

    cv::Mat imageData = cv::imread(imagePath, cv::IMREAD_UNCHANGED);
    if (imageData.empty()) {
        SuperDebug::Error("Failed to read image: {}", imagePath);
        throw std::runtime_error("Failed to read image: " + imagePath);
    }

    Image image(imageData, imagePath);
    _ApplyDefaultBlackNoData(image);
    return image;
}

} // namespace RSPIP::IO
