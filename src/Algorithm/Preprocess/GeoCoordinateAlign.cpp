#include "Algorithm/Preprocess/GeoCoordinateAlign.h"
#include "Util/Color.h"
#include "Util/ProjectionTransformer.h"
#include "Util/SuperDebug.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace RSPIP::Algorithm::PreprocessAlgorithm {

namespace {

constexpr double kEpsilon = 1e-9;

std::pair<double, double> _ApplyGeoTransform(const std::vector<double> &geoTransform, double row, double column) {
    return {
        geoTransform[3] + column * geoTransform[4] + row * geoTransform[5],
        geoTransform[0] + column * geoTransform[1] + row * geoTransform[2]};
}

bool _WorldToPixel(const std::vector<double> &geoTransform, double latitude, double longitude, double &row, double &column) {
    if (geoTransform.size() != 6) {
        return false;
    }

    const auto determinant = geoTransform[1] * geoTransform[5] - geoTransform[2] * geoTransform[4];
    if (std::abs(determinant) < kEpsilon) {
        return false;
    }

    const auto deltaX = longitude - geoTransform[0];
    const auto deltaY = latitude - geoTransform[3];

    column = (deltaX * geoTransform[5] - deltaY * geoTransform[2]) / determinant;
    row = (deltaY * geoTransform[1] - deltaX * geoTransform[4]) / determinant;
    return true;
}

std::array<std::pair<double, double>, 4> _GetImageCorners(const GeoImage &imageData) {
    return {
        _ApplyGeoTransform(imageData.GeoTransform, 0.0, 0.0),
        _ApplyGeoTransform(imageData.GeoTransform, 0.0, static_cast<double>(imageData.Width())),
        _ApplyGeoTransform(imageData.GeoTransform, static_cast<double>(imageData.Height()), 0.0),
        _ApplyGeoTransform(imageData.GeoTransform, static_cast<double>(imageData.Height()), static_cast<double>(imageData.Width()))};
}

ImageGeoBounds _BuildGeoBounds(const std::vector<double> &geoTransform, int rows, int columns) {
    const auto corners = std::array<std::pair<double, double>, 4>{
        _ApplyGeoTransform(geoTransform, 0.0, 0.0),
        _ApplyGeoTransform(geoTransform, 0.0, static_cast<double>(columns)),
        _ApplyGeoTransform(geoTransform, static_cast<double>(rows), 0.0),
        _ApplyGeoTransform(geoTransform, static_cast<double>(rows), static_cast<double>(columns))};

    ImageGeoBounds bounds = {
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::lowest()};

    for (const auto &[latitude, longitude] : corners) {
        bounds.MinLatitude = std::min(bounds.MinLatitude, latitude);
        bounds.MaxLatitude = std::max(bounds.MaxLatitude, latitude);
        bounds.MinLongitude = std::min(bounds.MinLongitude, longitude);
        bounds.MaxLongitude = std::max(bounds.MaxLongitude, longitude);
    }

    return bounds;
}

cv::Scalar _BuildFillValue(int imageType, const cv::Vec3b &nonData) {
    switch (CV_MAT_CN(imageType)) {
    case 1:
        return cv::Scalar(0);
    case 3:
        return cv::Scalar(nonData[0], nonData[1], nonData[2]);
    case 4:
        return cv::Scalar(nonData[0], nonData[1], nonData[2], 0);
    default:
        return cv::Scalar();
    }
}

void _SetImagePixelValue(cv::Mat &targetImage, int targetRow, int targetColumn, const GeoImage &sourceImage, int sourceRow, int sourceColumn) {
    switch (sourceImage.GetBandCounts()) {
    case 1:
        targetImage.at<unsigned char>(targetRow, targetColumn) = sourceImage.GetPixelValue<unsigned char>(sourceRow, sourceColumn);
        break;
    case 3:
        targetImage.at<cv::Vec3b>(targetRow, targetColumn) = sourceImage.GetPixelValue<cv::Vec3b>(sourceRow, sourceColumn);
        break;
    case 4:
        targetImage.at<cv::Vec4b>(targetRow, targetColumn) = sourceImage.GetPixelValue<cv::Vec4b>(sourceRow, sourceColumn);
        break;
    default:
        break;
    }
}

} // namespace

GeoCoordinateAlign::GeoCoordinateAlign(const std::vector<GeoImage> &imageDatas)
    : PreprocessAlgorithmBase(imageDatas) {}

GeoCoordinateAlign::GeoCoordinateAlign(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks)
    : PreprocessAlgorithmBase(imageDatas, cloudMasks) {}

void GeoCoordinateAlign::Execute() {
    if (_ImageDatas.empty()) {
        SuperDebug::Warn("GeoCoordinateAlign received no input images.");
        return;
    }

    _BuildUnifiedGrid();

    AlignedImages.clear();
    AlignedCloudMasks.clear();
    AlignedImages.reserve(_ImageDatas.size());
    if (!_CloudMasks.empty()) {
        AlignedCloudMasks.reserve(_CloudMasks.size());
    }

    const bool hasPairedMasks = !_CloudMasks.empty() && _CloudMasks.size() == _ImageDatas.size();
    if (!_CloudMasks.empty() && !hasPairedMasks) {
        SuperDebug::Warn("Cloud mask count does not match image count. Only images will be aligned.");
    }

    for (size_t index = 0; index < _ImageDatas.size(); ++index) {
        const auto localGrid = _BuildLocalGridWindow(_ImageDatas[index]);
        AlignedImages.emplace_back(_WarpGeoImage(_ImageDatas[index], localGrid));

        if (hasPairedMasks) {
            AlignedCloudMasks.emplace_back(_WarpCloudMask(_CloudMasks[index], localGrid));
        }
    }
}

void GeoCoordinateAlign::_BuildUnifiedGrid() {
    const auto &referenceImage = _ImageDatas.front();
    if (referenceImage.GeoTransform.size() != 6) {
        SuperDebug::Error("Reference image has no valid GeoTransform.");
        return;
    }

    const auto pixelWidth = std::abs(referenceImage.GeoTransform[1]);
    const auto pixelHeight = std::abs(referenceImage.GeoTransform[5]);
    if (pixelWidth < kEpsilon || pixelHeight < kEpsilon) {
        SuperDebug::Error("Reference image resolution is invalid.");
        return;
    }

    double minLongitude = std::numeric_limits<double>::max();
    double maxLongitude = std::numeric_limits<double>::lowest();
    double minLatitude = std::numeric_limits<double>::max();
    double maxLatitude = std::numeric_limits<double>::lowest();

    UnifiedGrid.Projection = referenceImage.Projection;

    for (const auto &imageData : _ImageDatas) {
        Util::ProjectionTransformer transformer(imageData.Projection, UnifiedGrid.Projection);
        if (!transformer.IsIdentity() && !transformer.IsValid()) {
            SuperDebug::Warn("Projection transform is unavailable for image: {}. Raw coordinates will be used.", imageData.ImageName);
        }

        for (auto [latitude, longitude] : _GetImageCorners(imageData)) {
            if (!transformer.IsIdentity() && transformer.IsValid()) {
                if (!transformer.Transform(latitude, longitude)) {
                    continue;
                }
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
        return;
    }

    UnifiedGrid.GeoTransform = {minLongitude, pixelWidth, 0.0, maxLatitude, 0.0, -pixelHeight};
    UnifiedGrid.Columns = std::max(1, static_cast<int>(std::ceil((maxLongitude - minLongitude) / pixelWidth)));
    UnifiedGrid.Rows = std::max(1, static_cast<int>(std::ceil((maxLatitude - minLatitude) / pixelHeight)));
}

GeoCoordinateAlign::LocalGridWindow GeoCoordinateAlign::_BuildLocalGridWindow(const GeoImage &imageData) const {
    LocalGridWindow localGrid = {};

    Util::ProjectionTransformer transformer(imageData.Projection, UnifiedGrid.Projection);

    double minRow = std::numeric_limits<double>::max();
    double maxRow = std::numeric_limits<double>::lowest();
    double minColumn = std::numeric_limits<double>::max();
    double maxColumn = std::numeric_limits<double>::lowest();

    for (auto [latitude, longitude] : _GetImageCorners(imageData)) {
        if (!transformer.IsIdentity() && transformer.IsValid()) {
            if (!transformer.Transform(latitude, longitude)) {
                continue;
            }
        }

        double row = 0.0;
        double column = 0.0;
        if (!_WorldToPixel(UnifiedGrid.GeoTransform, latitude, longitude, row, column)) {
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

    const auto alignedMinRow = std::floor(minRow);
    const auto alignedMaxRow = std::ceil(maxRow);
    const auto alignedMinColumn = std::floor(minColumn);
    const auto alignedMaxColumn = std::ceil(maxColumn);

    auto [originLatitude, originLongitude] = _ApplyGeoTransform(UnifiedGrid.GeoTransform, alignedMinRow, alignedMinColumn);
    localGrid.GeoTransform = {originLongitude, UnifiedGrid.GeoTransform[1], 0.0, originLatitude, 0.0, UnifiedGrid.GeoTransform[5]};
    localGrid.Rows = std::max(1, static_cast<int>(alignedMaxRow - alignedMinRow));
    localGrid.Columns = std::max(1, static_cast<int>(alignedMaxColumn - alignedMinColumn));

    return localGrid;
}

GeoImage GeoCoordinateAlign::_WarpGeoImage(const GeoImage &imageData, const LocalGridWindow &localGrid) const {
    cv::Mat alignedImage(localGrid.Rows, localGrid.Columns, imageData.GetImageType(), _BuildFillValue(imageData.GetImageType(), imageData.NonData));
    Util::ProjectionTransformer targetToSource(UnifiedGrid.Projection, imageData.Projection);

    if (!targetToSource.IsIdentity() && !targetToSource.IsValid()) {
        SuperDebug::Warn("Projection transform is unavailable while warping image: {}. Raw coordinates will be used.", imageData.ImageName);
    }

    for (int row = 0; row < localGrid.Rows; ++row) {
        for (int column = 0; column < localGrid.Columns; ++column) {
            auto [latitude, longitude] = _ApplyGeoTransform(localGrid.GeoTransform, row, column);
            if (!targetToSource.IsIdentity() && targetToSource.IsValid()) {
                if (!targetToSource.Transform(latitude, longitude)) {
                    continue;
                }
            }

            double sourceRow = 0.0;
            double sourceColumn = 0.0;
            if (!_WorldToPixel(imageData.GeoTransform, latitude, longitude, sourceRow, sourceColumn)) {
                continue;
            }

            const auto nearestRow = static_cast<int>(std::lround(sourceRow));
            const auto nearestColumn = static_cast<int>(std::lround(sourceColumn));
            if (imageData.IsOutOfBounds(nearestRow, nearestColumn)) {
                continue;
            }

            _SetImagePixelValue(alignedImage, row, column, imageData, nearestRow, nearestColumn);
        }
    }

    GeoImage result(alignedImage, imageData.ImageName);
    result.NonData = imageData.NonData;
    result.GeoTransform = localGrid.GeoTransform;
    result.Projection = UnifiedGrid.Projection;
    result.ImageBounds = _BuildGeoBounds(localGrid.GeoTransform, localGrid.Rows, localGrid.Columns);
    return result;
}

CloudMask GeoCoordinateAlign::_WarpCloudMask(const CloudMask &cloudMask, const LocalGridWindow &localGrid) const {
    cv::Mat alignedMask(localGrid.Rows, localGrid.Columns, cloudMask.GetImageType(), cv::Scalar(0));
    Util::ProjectionTransformer targetToSource(UnifiedGrid.Projection, cloudMask.Projection);

    if (!targetToSource.IsIdentity() && !targetToSource.IsValid()) {
        SuperDebug::Warn("Projection transform is unavailable while warping cloud mask: {}. Raw coordinates will be used.", cloudMask.ImageName);
    }

    for (int row = 0; row < localGrid.Rows; ++row) {
        for (int column = 0; column < localGrid.Columns; ++column) {
            auto [latitude, longitude] = _ApplyGeoTransform(localGrid.GeoTransform, row, column);
            if (!targetToSource.IsIdentity() && targetToSource.IsValid()) {
                if (!targetToSource.Transform(latitude, longitude)) {
                    continue;
                }
            }

            double sourceRow = 0.0;
            double sourceColumn = 0.0;
            if (!_WorldToPixel(cloudMask.GeoTransform, latitude, longitude, sourceRow, sourceColumn)) {
                continue;
            }

            const auto nearestRow = static_cast<int>(std::lround(sourceRow));
            const auto nearestColumn = static_cast<int>(std::lround(sourceColumn));
            if (cloudMask.IsOutOfBounds(nearestRow, nearestColumn)) {
                continue;
            }

            alignedMask.at<unsigned char>(row, column) = cloudMask.GetPixelValue<unsigned char>(nearestRow, nearestColumn);
        }
    }

    CloudMask result(alignedMask, cloudMask.ImageName);
    result.GeoTransform = localGrid.GeoTransform;
    result.Projection = UnifiedGrid.Projection;
    result.ImageBounds = _BuildGeoBounds(localGrid.GeoTransform, localGrid.Rows, localGrid.Columns);
    return result;
}

} // namespace RSPIP::Algorithm::PreprocessAlgorithm
