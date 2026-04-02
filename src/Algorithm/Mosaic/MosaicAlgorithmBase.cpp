#include "Algorithm/Mosaic/MosaicAlgorithmBase.h"
#include "Algorithm/Detail/AlgorithmValidation.h"
#include "Util/SuperDebug.hpp"
#include <cmath>
#include <limits>
#include <omp.h>

namespace RSPIP::Algorithm::MosaicAlgorithm {

MosaicAlgorithmBase::MosaicAlgorithmBase(std::vector<Image> imageDatas)
    : AlgorithmResult(), _MosaicImages(std::move(imageDatas)) {
    _InitMosaicParameters();
}

void MosaicAlgorithmBase::_InitMosaicParameters() {
    if (!_GetGeoInfo()) {
        Detail::ResetImageResult(AlgorithmResult);
        return;
    }
    _GetDimensions();
}

bool MosaicAlgorithmBase::_GetGeoInfo() {
    if (_MosaicImages.empty()) {
        SuperDebug::Warn("No mosaic images were provided.");
        return false;
    }

    for (const auto &imageData : _MosaicImages) {
        if (!Detail::ValidateGeoImage(imageData, "MosaicAlgorithmBase", "input image") ||
            !Detail::ValidateBgrImage(imageData, "MosaicAlgorithmBase", "input image")) {
            return false;
        }
    }

    GeoInfo geoInfo = {};
    geoInfo.Bounds.MinLongitude = std::numeric_limits<double>::max();
    geoInfo.Bounds.MinLatitude = std::numeric_limits<double>::max();
    geoInfo.Bounds.MaxLongitude = std::numeric_limits<double>::lowest();
    geoInfo.Bounds.MaxLatitude = std::numeric_limits<double>::lowest();

    for (const auto &imageData : _MosaicImages) {
        geoInfo.Bounds.MinLongitude = std::min(geoInfo.Bounds.MinLongitude, imageData.GeoInfo->Bounds.MinLongitude);
        geoInfo.Bounds.MaxLongitude = std::max(geoInfo.Bounds.MaxLongitude, imageData.GeoInfo->Bounds.MaxLongitude);
        geoInfo.Bounds.MinLatitude = std::min(geoInfo.Bounds.MinLatitude, imageData.GeoInfo->Bounds.MinLatitude);
        geoInfo.Bounds.MaxLatitude = std::max(geoInfo.Bounds.MaxLatitude, imageData.GeoInfo->Bounds.MaxLatitude);
    }

    geoInfo.GeoTransform = {
        geoInfo.Bounds.MinLongitude,
        _MosaicImages.front().GeoInfo->GeoTransform[1],
        0.0,
        geoInfo.Bounds.MaxLatitude,
        0.0,
        _MosaicImages.front().GeoInfo->GeoTransform[5]};
    geoInfo.Projection = _MosaicImages.front().GeoInfo->Projection;

    AlgorithmResult.GeoInfo = std::move(geoInfo);
    return true;
}

void MosaicAlgorithmBase::_GetDimensions() {
    if (!AlgorithmResult.GeoInfo.has_value()) {
        return;
    }

    const auto &geoInfo = *AlgorithmResult.GeoInfo;
    if (std::abs(geoInfo.GeoTransform[1]) < 1e-9 || std::abs(geoInfo.GeoTransform[5]) < 1e-9) {
        SuperDebug::Error("Invalid GeoTransform resolution");
        Detail::ResetImageResult(AlgorithmResult);
        return;
    }

    const auto mosaicCols = std::max(1, static_cast<int>(std::ceil((geoInfo.Bounds.MaxLongitude - geoInfo.Bounds.MinLongitude) / std::abs(geoInfo.GeoTransform[1]))) + 1);
    const auto mosaicRows = std::max(1, static_cast<int>(std::ceil((geoInfo.Bounds.MaxLatitude - geoInfo.Bounds.MinLatitude) / std::abs(geoInfo.GeoTransform[5]))) + 1);

    AlgorithmResult.NoDataValues = {0.0, 0.0, 0.0};
    AlgorithmResult.ImageData = cv::Mat(mosaicRows, mosaicCols, CV_8UC3, AlgorithmResult.GetFillScalarForOpenCV());
    AlgorithmResult.GeoInfo->RebuildBounds(AlgorithmResult.Height(), AlgorithmResult.Width());
}

void MosaicAlgorithmBase::_PasteImageToMosaicResult(const Image &imageData) {
    if (!imageData.GeoInfo.has_value() || !AlgorithmResult.GeoInfo.has_value()) {
        return;
    }

    const int columns = imageData.Width();
    const int rows = imageData.Height();

#pragma omp parallel for
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            const auto &pixelValue = imageData.GetPixelValue<cv::Vec3b>(row, col);
            if (imageData.IsNoDataPixel(row, col)) {
                continue;
            }

            const auto [latitude, longitude] = imageData.GeoInfo->GetLatLon(row, col);
            const auto [mosaicRow, mosaicColumn] = AlgorithmResult.GeoInfo->LatLonToRC(latitude, longitude);
            if (mosaicRow == -1 || mosaicColumn == -1) {
                continue;
            }

            AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
        }
    }
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm
