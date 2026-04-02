#include "Algorithm/Preprocess/GeoCoordinateAlign.h"
#include "Algorithm/Detail/AlgorithmValidation.h"
#include "Util/ProjectionTransformer.h"
#include "Util/SuperDebug.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <opencv2/imgproc.hpp>

namespace RSPIP::Algorithm::PreprocessAlgorithm {

namespace {

constexpr double kEpsilon = 1e-9;
constexpr unsigned char kMaskFillValue = 255;

std::pair<double, double> _ApplyGeoTransform(const std::vector<double> &geoTransform, double row, double column) {
    return {
        geoTransform[3] + column * geoTransform[4] + row * geoTransform[5],
        geoTransform[0] + column * geoTransform[1] + row * geoTransform[2]};
}

std::array<std::pair<double, double>, 4> _GetImageCorners(const Image &imageData) {
    const double maxRow = static_cast<double>(std::max(imageData.Height() - 1, 0));
    const double maxColumn = static_cast<double>(std::max(imageData.Width() - 1, 0));
    return {
        _ApplyGeoTransform(imageData.GeoInfo->GeoTransform, 0.0, 0.0),
        _ApplyGeoTransform(imageData.GeoInfo->GeoTransform, 0.0, maxColumn),
        _ApplyGeoTransform(imageData.GeoInfo->GeoTransform, maxRow, 0.0),
        _ApplyGeoTransform(imageData.GeoInfo->GeoTransform, maxRow, maxColumn)};
}

bool _HasGeoInfo(const Image &imageData) {
    return imageData.GeoInfo.has_value() && imageData.GeoInfo->HasValidTransform();
}

bool _SameGeoTransform(const std::vector<double> &left, const std::vector<double> &right) {
    if (left.size() != right.size()) {
        return false;
    }

    for (size_t index = 0; index < left.size(); ++index) {
        if (std::abs(left[index] - right[index]) > kEpsilon) {
            return false;
        }
    }

    return true;
}

} // namespace

GeoCoordinateAlign::GeoCoordinateAlign(std::vector<Image> imageDatas)
    : PreprocessAlgorithmBase(std::move(imageDatas)) {}

GeoCoordinateAlign::GeoCoordinateAlign(std::vector<Image> imageDatas, std::vector<Image> maskImages)
    : PreprocessAlgorithmBase(std::move(imageDatas), std::move(maskImages)) {}

void GeoCoordinateAlign::Execute() {
    AlignedImages.clear();
    AlignedMaskImages.clear();

    if (_ImageDatas.empty()) {
        SuperDebug::Warn("GeoCoordinateAlign received no input images.");
        return;
    }

    if (!_BuildUnifiedGrid()) {
        return;
    }

    AlignedImages.reserve(_ImageDatas.size());
    if (!_MaskImages.empty()) {
        AlignedMaskImages.reserve(_MaskImages.size());
    }

    const bool hasPairedMasks = !_MaskImages.empty() && _MaskImages.size() == _ImageDatas.size();
    if (!_MaskImages.empty() && !hasPairedMasks) {
        SuperDebug::Warn("Mask image count does not match image count. Only images will be aligned.");
    }

    for (size_t index = 0; index < _ImageDatas.size(); ++index) {
        if (!Detail::ValidateGeoImage(_ImageDatas[index], "GeoCoordinateAlign", "input image")) {
            AlignedImages.clear();
            AlignedMaskImages.clear();
            return;
        }

        const auto localGrid = _BuildLocalGridWindow(_ImageDatas[index]);
        const auto imageWarp = _PrepareWarp(_ImageDatas[index], localGrid);
        AlignedImages.emplace_back(_WarpImage(_ImageDatas[index], imageWarp));

        if (!hasPairedMasks) {
            continue;
        }

        if (!Detail::ValidateGeoImage(_MaskImages[index], "GeoCoordinateAlign", "mask image")) {
            AlignedImages.clear();
            AlignedMaskImages.clear();
            return;
        }

        if (_CanReuseWarpPreparation(_ImageDatas[index], _MaskImages[index])) {
            AlignedMaskImages.emplace_back(_WarpMaskImage(_MaskImages[index], imageWarp));
        } else {
            AlignedMaskImages.emplace_back(_WarpMaskImage(_MaskImages[index], _PrepareWarp(_MaskImages[index], localGrid)));
        }
    }
}

bool GeoCoordinateAlign::_BuildUnifiedGrid() {
    const auto &referenceImage = _ImageDatas.front();
    if (!_HasGeoInfo(referenceImage)) {
        SuperDebug::Error("Reference image has no valid GeoInfo.");
        return false;
    }

    const auto pixelWidth = std::abs(referenceImage.GeoInfo->GeoTransform[1]);
    const auto pixelHeight = std::abs(referenceImage.GeoInfo->GeoTransform[5]);
    if (pixelWidth < kEpsilon || pixelHeight < kEpsilon) {
        SuperDebug::Error("Reference image resolution is invalid.");
        return false;
    }

    double minLongitude = std::numeric_limits<double>::max();
    double maxLongitude = std::numeric_limits<double>::lowest();
    double minLatitude = std::numeric_limits<double>::max();
    double maxLatitude = std::numeric_limits<double>::lowest();
    UnifiedGrid.Projection = referenceImage.GeoInfo->Projection;

    for (const auto &imageData : _ImageDatas) {
        if (!_HasGeoInfo(imageData)) {
            SuperDebug::Error("GeoCoordinateAlign requires GeoInfo for image: {}", imageData.ImageName);
            return false;
        }

        Util::ProjectionTransformer transformer(imageData.GeoInfo->Projection, UnifiedGrid.Projection);
        if (!transformer.IsIdentity() && !transformer.IsValid()) {
            SuperDebug::Warn("Projection transform is unavailable for image: {}. Raw coordinates will be used.", imageData.ImageName);
        }

        for (auto [latitude, longitude] : _GetImageCorners(imageData)) {
            if (!transformer.IsIdentity() && transformer.IsValid() && !transformer.Transform(latitude, longitude)) {
                continue;
            }

            minLongitude = std::min(minLongitude, longitude);
            maxLongitude = std::max(maxLongitude, longitude);
            minLatitude = std::min(minLatitude, latitude);
            maxLatitude = std::max(maxLatitude, latitude);
        }
    }

    if (!std::isfinite(minLongitude) || !std::isfinite(maxLongitude) ||
        !std::isfinite(minLatitude) || !std::isfinite(maxLatitude)) {
        SuperDebug::Error("Failed to build unified grid bounds.");
        return false;
    }

    UnifiedGrid.GeoTransform = {minLongitude, pixelWidth, 0.0, maxLatitude, 0.0, -pixelHeight};
    UnifiedGrid.Columns = std::max(1, static_cast<int>(std::ceil((maxLongitude - minLongitude) / pixelWidth)) + 1);
    UnifiedGrid.Rows = std::max(1, static_cast<int>(std::ceil((maxLatitude - minLatitude) / pixelHeight)) + 1);
    return true;
}

GeoCoordinateAlign::LocalGridWindow GeoCoordinateAlign::_BuildLocalGridWindow(const Image &imageData) const {
    LocalGridWindow localGrid = {};
    Util::ProjectionTransformer transformer(imageData.GeoInfo->Projection, UnifiedGrid.Projection);
    GeoInfo unifiedGridInfo = {};
    unifiedGridInfo.GeoTransform = UnifiedGrid.GeoTransform;
    unifiedGridInfo.Projection = UnifiedGrid.Projection;
    unifiedGridInfo.RebuildBounds(UnifiedGrid.Rows, UnifiedGrid.Columns);

    double minRow = std::numeric_limits<double>::max();
    double maxRow = std::numeric_limits<double>::lowest();
    double minColumn = std::numeric_limits<double>::max();
    double maxColumn = std::numeric_limits<double>::lowest();

    for (auto [latitude, longitude] : _GetImageCorners(imageData)) {
        if (!transformer.IsIdentity() && transformer.IsValid() && !transformer.Transform(latitude, longitude)) {
            continue;
        }

        double row = 0.0;
        double column = 0.0;
        if (!unifiedGridInfo.TryWorldToPixel(latitude, longitude, row, column)) {
            continue;
        }

        minRow = std::min(minRow, row);
        maxRow = std::max(maxRow, row);
        minColumn = std::min(minColumn, column);
        maxColumn = std::max(maxColumn, column);
    }

    if (!std::isfinite(minRow) || !std::isfinite(maxRow) ||
        !std::isfinite(minColumn) || !std::isfinite(maxColumn)) {
        SuperDebug::Warn("Failed to compute local grid window for image: {}. Falling back to unified full grid.", imageData.ImageName);
        localGrid.GeoTransform = UnifiedGrid.GeoTransform;
        localGrid.Rows = UnifiedGrid.Rows;
        localGrid.Columns = UnifiedGrid.Columns;
        return localGrid;
    }

    const int alignedMinRow = static_cast<int>(std::floor(minRow));
    const int alignedMaxRow = static_cast<int>(std::ceil(maxRow));
    const int alignedMinColumn = static_cast<int>(std::floor(minColumn));
    const int alignedMaxColumn = static_cast<int>(std::ceil(maxColumn));

    const auto [originLatitude, originLongitude] = _ApplyGeoTransform(UnifiedGrid.GeoTransform, alignedMinRow, alignedMinColumn);
    localGrid.GeoTransform = {originLongitude, UnifiedGrid.GeoTransform[1], 0.0, originLatitude, 0.0, UnifiedGrid.GeoTransform[5]};
    localGrid.Rows = std::max(1, alignedMaxRow - alignedMinRow + 1);
    localGrid.Columns = std::max(1, alignedMaxColumn - alignedMinColumn + 1);
    return localGrid;
}

bool GeoCoordinateAlign::_CanReuseWarpPreparation(const Image &imageData, const Image &otherImage) const {
    return imageData.Height() == otherImage.Height() &&
           imageData.Width() == otherImage.Width() &&
           _HasGeoInfo(imageData) &&
           _HasGeoInfo(otherImage) &&
           imageData.GeoInfo->Projection == otherImage.GeoInfo->Projection &&
           _SameGeoTransform(imageData.GeoInfo->GeoTransform, otherImage.GeoInfo->GeoTransform);
}

GeoCoordinateAlign::WarpPreparation GeoCoordinateAlign::_PrepareWarp(const Image &imageData, const LocalGridWindow &localGrid) const {
    WarpPreparation warpPreparation = {};
    warpPreparation.LocalGrid = localGrid;
    warpPreparation.MapX = cv::Mat(localGrid.Rows, localGrid.Columns, CV_32FC1, cv::Scalar(-1.0f));
    warpPreparation.MapY = cv::Mat(localGrid.Rows, localGrid.Columns, CV_32FC1, cv::Scalar(-1.0f));

    Util::ProjectionTransformer targetToSource(UnifiedGrid.Projection, imageData.GeoInfo->Projection);
    if (!targetToSource.IsIdentity() && !targetToSource.IsValid()) {
        SuperDebug::Warn("Projection transform is unavailable while warping image: {}. Raw coordinates will be used.", imageData.ImageName);
    }

    for (int row = 0; row < localGrid.Rows; ++row) {
        float *mapXPtr = warpPreparation.MapX.ptr<float>(row);
        float *mapYPtr = warpPreparation.MapY.ptr<float>(row);
        for (int column = 0; column < localGrid.Columns; ++column) {
            auto [latitude, longitude] = _ApplyGeoTransform(localGrid.GeoTransform, row, column);
            if (!targetToSource.IsIdentity() && targetToSource.IsValid() && !targetToSource.Transform(latitude, longitude)) {
                continue;
            }

            double sourceRow = 0.0;
            double sourceColumn = 0.0;
            if (!imageData.GeoInfo->TryWorldToPixel(latitude, longitude, sourceRow, sourceColumn)) {
                continue;
            }

            if (sourceRow < 0.0 || sourceRow >= static_cast<double>(imageData.Height()) ||
                sourceColumn < 0.0 || sourceColumn >= static_cast<double>(imageData.Width())) {
                continue;
            }

            mapXPtr[column] = static_cast<float>(sourceColumn);
            mapYPtr[column] = static_cast<float>(sourceRow);
        }
    }

    return warpPreparation;
}

Image GeoCoordinateAlign::_WarpImage(const Image &imageData, const WarpPreparation &warpPreparation) const {
    cv::Mat alignedImage;
    cv::remap(
        imageData.ImageData,
        alignedImage,
        warpPreparation.MapX,
        warpPreparation.MapY,
        cv::INTER_NEAREST,
        cv::BORDER_CONSTANT,
        imageData.GetFillScalarForOpenCV());

    Image result(alignedImage, imageData.ImageName);
    result.NoDataValues = imageData.NoDataValues;
    result.GeoInfo = GeoInfo{warpPreparation.LocalGrid.GeoTransform, UnifiedGrid.Projection, {}};
    result.GeoInfo->RebuildBounds(result.Height(), result.Width());
    return result;
}

Image GeoCoordinateAlign::_WarpMaskImage(const Image &maskImage, const WarpPreparation &warpPreparation) const {
    cv::Mat alignedMask;
    cv::remap(
        maskImage.ImageData,
        alignedMask,
        warpPreparation.MapX,
        warpPreparation.MapY,
        cv::INTER_NEAREST,
        cv::BORDER_CONSTANT,
        cv::Scalar::all(kMaskFillValue));

    Image result(alignedMask, maskImage.ImageName);
    result.NoDataValues = maskImage.NoDataValues;
    result.GeoInfo = GeoInfo{warpPreparation.LocalGrid.GeoTransform, UnifiedGrid.Projection, {}};
    result.GeoInfo->RebuildBounds(result.Height(), result.Width());
    return result;
}

} // namespace RSPIP::Algorithm::PreprocessAlgorithm
