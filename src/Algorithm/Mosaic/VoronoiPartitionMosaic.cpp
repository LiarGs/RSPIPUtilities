#include "Algorithm/Mosaic/VoronoiPartitionMosaic.h"
#include "Algorithm/Detail/AlgorithmValidation.h"
#include "Algorithm/Detail/MaskPolicies.h"
#include "Basic/RegionExtraction.h"
#include "Util/SuperDebug.hpp"
#include <cmath>
#include <limits>

namespace RSPIP::Algorithm::MosaicAlgorithm {

namespace {

constexpr auto kAlgorithmName = "VoronoiPartitionMosaic";
constexpr double kDistanceEpsilon = 1e-9;

} // namespace

VoronoiPartitionMosaic::VoronoiPartitionMosaic(std::vector<Image> imageDatas)
    : MosaicAlgorithmBase(std::move(imageDatas)), _Inputs(), _HasMaskInputs(false), _ProvidedMaskCount(0) {
    _Inputs.reserve(_MosaicImages.size());
    for (size_t index = 0; index < _MosaicImages.size(); ++index) {
        InputBundle input = {};
        input.SourceImage = _MosaicImages[index];
        input.OriginalIndex = index;
        _Inputs.emplace_back(std::move(input));
    }
}

VoronoiPartitionMosaic::VoronoiPartitionMosaic(std::vector<Image> imageDatas, std::vector<Image> maskImages)
    : MosaicAlgorithmBase(std::move(imageDatas)), _Inputs(), _HasMaskInputs(!maskImages.empty()), _ProvidedMaskCount(maskImages.size()) {
    _Inputs.reserve(_MosaicImages.size());
    for (size_t index = 0; index < _MosaicImages.size(); ++index) {
        InputBundle input = {};
        input.SourceImage = _MosaicImages[index];
        if (index < maskImages.size()) {
            input.Mask = std::move(maskImages[index]);
        }
        input.OriginalIndex = index;
        _Inputs.emplace_back(std::move(input));
    }
}

void VoronoiPartitionMosaic::Execute() {
    if (!_ValidateExecuteInputs()) {
        Detail::ResetImageResult(AlgorithmResult);
        return;
    }

    AlgorithmResult.ImageData.setTo(AlgorithmResult.GetFillScalarForOpenCV());
    SuperDebug::Info("{} mosaic image size: {} x {}", kAlgorithmName, AlgorithmResult.Height(), AlgorithmResult.Width());

    if (!_PrepareInputs()) {
        Detail::ResetImageResult(AlgorithmResult);
        return;
    }

    int enabledInputCount = 0;
    for (const auto &input : _Inputs) {
        if (input.IsEnabled) {
            ++enabledInputCount;
        }
    }

    if (enabledInputCount == 0) {
        SuperDebug::Warn("{} found no enabled inputs after valid-region preparation. The result will remain NoData.", kAlgorithmName);
        return;
    }

    int filledPixelCount = 0;
    for (int mosaicRow = 0; mosaicRow < AlgorithmResult.Height(); ++mosaicRow) {
        for (int mosaicColumn = 0; mosaicColumn < AlgorithmResult.Width(); ++mosaicColumn) {
            const auto [latitude, longitude] = AlgorithmResult.GeoInfo->GetLatLon(mosaicRow, mosaicColumn);

            const InputBundle *bestInput = nullptr;
            cv::Vec3b bestPixel = {};
            double bestDistanceSquared = std::numeric_limits<double>::max();

            for (const auto &input : _Inputs) {
                if (!input.IsEnabled) {
                    continue;
                }

                cv::Vec3b pixelValue = {};
                if (!_TryResolveSourcePixelForWorldLocation(input, latitude, longitude, pixelValue)) {
                    continue;
                }

                const double distanceSquared = _ComputeSeedDistanceSquared(input, mosaicRow, mosaicColumn);
                if (bestInput == nullptr ||
                    distanceSquared + kDistanceEpsilon < bestDistanceSquared ||
                    (std::abs(distanceSquared - bestDistanceSquared) <= kDistanceEpsilon && input.OriginalIndex < bestInput->OriginalIndex)) {
                    bestInput = &input;
                    bestPixel = pixelValue;
                    bestDistanceSquared = distanceSquared;
                }
            }

            if (bestInput != nullptr) {
                AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, bestPixel);
                ++filledPixelCount;
            }
        }
    }

    SuperDebug::Info("{} completed. Enabled inputs={}, filled_pixels={}.", kAlgorithmName, enabledInputCount, filledPixelCount);
}

bool VoronoiPartitionMosaic::_ValidateExecuteInputs() const {
    if (_MosaicImages.empty() || AlgorithmResult.ImageData.empty() || !AlgorithmResult.GeoInfo.has_value()) {
        SuperDebug::Error("{} could not execute because mosaic initialization is invalid.", kAlgorithmName);
        return false;
    }

    if (!_HasMaskInputs) {
        return true;
    }

    if (_ProvidedMaskCount != _Inputs.size()) {
        SuperDebug::Error("{} requires mask count to match input image count. images={}, masks={}.",
                          kAlgorithmName,
                          _Inputs.size(),
                          _ProvidedMaskCount);
        return false;
    }

    for (size_t index = 0; index < _Inputs.size(); ++index) {
        const auto &mask = _Inputs[index].Mask;
        const auto &image = _Inputs[index].SourceImage;
        if (mask.ImageData.empty()) {
            SuperDebug::Error("{} requires a non-empty mask image at index {}.", kAlgorithmName, index);
            return false;
        }

        if (mask.Height() != image.Height() || mask.Width() != image.Width()) {
            SuperDebug::Error("{} requires mask {} to match image {} size. image={}x{}, mask={}x{}.",
                              kAlgorithmName,
                              index,
                              index,
                              image.Width(),
                              image.Height(),
                              mask.Width(),
                              mask.Height());
            return false;
        }
    }

    return true;
}

bool VoronoiPartitionMosaic::_PrepareInputs() {
    if (!_HasMaskInputs) {
        SuperDebug::Warn("{} is running without mask inputs and will exclude NoData only.", kAlgorithmName);
    }

    for (auto &input : _Inputs) {
        input.SelectionMask.release();
        input.ValidProjectedPixelCount = 0;
        input.SeedRow = 0.0;
        input.SeedColumn = 0.0;
        input.IsEnabled = true;
        input.DisabledReason.clear();

        if (!_BuildSelectionMask(input)) {
            return false;
        }

        double sumSeedRows = 0.0;
        double sumSeedColumns = 0.0;
        for (int sourceRow = 0; sourceRow < input.SourceImage.Height(); ++sourceRow) {
            for (int sourceColumn = 0; sourceColumn < input.SourceImage.Width(); ++sourceColumn) {
                int mosaicRow = -1;
                int mosaicColumn = -1;
                if (!_TryProjectSourcePixelToMosaic(input, sourceRow, sourceColumn, mosaicRow, mosaicColumn)) {
                    continue;
                }

                sumSeedRows += static_cast<double>(mosaicRow);
                sumSeedColumns += static_cast<double>(mosaicColumn);
                ++input.ValidProjectedPixelCount;
            }
        }

        if (input.ValidProjectedPixelCount == 0) {
            input.IsEnabled = false;
            input.DisabledReason = "no_valid_projected_pixels";
            SuperDebug::Warn("{} input {} has no valid projected pixels and will be disabled.",
                             kAlgorithmName,
                             input.OriginalIndex);
            continue;
        }

        const double projectedPixelCount = static_cast<double>(input.ValidProjectedPixelCount);
        input.SeedRow = sumSeedRows / projectedPixelCount;
        input.SeedColumn = sumSeedColumns / projectedPixelCount;
        SuperDebug::Info("{} input {} prepared: valid_projected_pixels={}, seed_center=({:.3f}, {:.3f}).",
                         kAlgorithmName,
                         input.OriginalIndex,
                         input.ValidProjectedPixelCount,
                         input.SeedRow,
                         input.SeedColumn);
    }

    return true;
}

bool VoronoiPartitionMosaic::_BuildSelectionMask(InputBundle &input) const {
    if (!_HasMaskInputs) {
        input.SelectionMask.release();
        return true;
    }

    const auto maskPolicy = Detail::DefaultBinaryMaskPolicy();
    input.SelectionMask = BuildSelectionMask(input.Mask, maskPolicy);
    if (!input.SelectionMask.empty()) {
        return true;
    }

    SuperDebug::Error("{} failed to build a selection mask for input {}.", kAlgorithmName, input.OriginalIndex);
    return false;
}

bool VoronoiPartitionMosaic::_IsSelectedByMask(const cv::Mat &selectionMask, int row, int column) const {
    if (selectionMask.empty()) {
        return false;
    }

    return selectionMask.at<unsigned char>(row, column) != 0;
}

bool VoronoiPartitionMosaic::_IsSourcePixelValid(const InputBundle &input, int row, int column) const {
    if (row < 0 || row >= input.SourceImage.Height() || column < 0 || column >= input.SourceImage.Width()) {
        return false;
    }

    if (input.SourceImage.IsNoDataPixel(row, column)) {
        return false;
    }

    return !_IsSelectedByMask(input.SelectionMask, row, column);
}

bool VoronoiPartitionMosaic::_TryProjectSourcePixelToMosaic(const InputBundle &input, int sourceRow, int sourceColumn, int &mosaicRow, int &mosaicColumn) const {
    if (!input.SourceImage.GeoInfo.has_value() || !AlgorithmResult.GeoInfo.has_value()) {
        return false;
    }

    if (!_IsSourcePixelValid(input, sourceRow, sourceColumn)) {
        return false;
    }

    const auto [latitude, longitude] = input.SourceImage.GeoInfo->GetLatLon(sourceRow, sourceColumn);
    const auto [targetRow, targetColumn] = AlgorithmResult.GeoInfo->LatLonToRC(latitude, longitude);
    if (targetRow == -1 || targetColumn == -1) {
        return false;
    }

    mosaicRow = targetRow;
    mosaicColumn = targetColumn;
    return true;
}

bool VoronoiPartitionMosaic::_TryResolveSourcePixelForWorldLocation(const InputBundle &input, double latitude, double longitude, cv::Vec3b &pixelValue) const {
    if (!input.SourceImage.GeoInfo.has_value()) {
        return false;
    }

    const auto [sourceRow, sourceColumn] = input.SourceImage.GeoInfo->LatLonToRC(latitude, longitude);
    if (sourceRow == -1 || sourceColumn == -1) {
        return false;
    }

    if (!_IsSourcePixelValid(input, sourceRow, sourceColumn)) {
        return false;
    }

    pixelValue = input.SourceImage.GetPixelValue<cv::Vec3b>(sourceRow, sourceColumn);
    return true;
}

double VoronoiPartitionMosaic::_ComputeSeedDistanceSquared(const InputBundle &input, int mosaicRow, int mosaicColumn) const {
    const double deltaRow = static_cast<double>(mosaicRow) - input.SeedRow;
    const double deltaColumn = static_cast<double>(mosaicColumn) - input.SeedColumn;
    return deltaRow * deltaRow + deltaColumn * deltaColumn;
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm
