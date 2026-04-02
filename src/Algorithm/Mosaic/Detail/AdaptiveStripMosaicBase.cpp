#include "Algorithm/Mosaic/Detail/AdaptiveStripMosaicBase.h"
#include "Algorithm/Detail/MaskPolicies.h"
#include "Basic/RegionExtraction.h"
#include "Util/General.h"
#include "Util/SuperDebug.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <format>
#include <limits>
#include <opencv2/imgproc.hpp>
#include <stack>

namespace RSPIP::Algorithm::MosaicAlgorithm::Detail {

namespace {

constexpr unsigned char kFilledValue = 255;
constexpr double kCorrelationEpsilon = 1e-9;

cv::Mat _CreateConstantMask(const Image &image, unsigned char value) {
    return cv::Mat(image.Height(), image.Width(), CV_8UC1, cv::Scalar(value));
}

unsigned char _GetSelectionMaskValue(const cv::Mat &selectionMask, int row, int column) {
    if (selectionMask.empty() || row < 0 || row >= selectionMask.rows || column < 0 || column >= selectionMask.cols) {
        return kFilledValue;
    }
    return selectionMask.at<unsigned char>(row, column);
}

bool _RectIsPreferred(const cv::Rect &candidate, const cv::Rect &currentBest) {
    if (candidate.area() != currentBest.area()) {
        return candidate.area() > currentBest.area();
    }
    if (candidate.y != currentBest.y) {
        return candidate.y < currentBest.y;
    }
    if (candidate.x != currentBest.x) {
        return candidate.x < currentBest.x;
    }
    if (candidate.height != currentBest.height) {
        return candidate.height > currentBest.height;
    }
    return candidate.width > currentBest.width;
}

double _ElapsedMilliseconds(const std::chrono::steady_clock::time_point &start, const std::chrono::steady_clock::time_point &end) {
    return std::chrono::duration<double, std::milli>(end - start).count();
}

double _ComputePearsonCorrelation(size_t sampleCount, double sumX, double sumY, double sumXX, double sumYY, double sumXY) {
    if (sampleCount <= 1) {
        return 0.0;
    }

    const auto count = static_cast<double>(sampleCount);
    const double numerator = sumXY - (sumX * sumY) / count;
    const double varianceX = sumXX - (sumX * sumX) / count;
    const double varianceY = sumYY - (sumY * sumY) / count;
    if (varianceX <= kCorrelationEpsilon || varianceY <= kCorrelationEpsilon) {
        return 0.0;
    }

    return numerator / std::sqrt(varianceX * varianceY);
}

} // namespace

AdaptiveStripMosaicBase::AdaptiveStripMosaicBase(std::vector<Image> imageDatas, std::vector<Image> maskImages)
    : MosaicAlgorithmBase(std::move(imageDatas)),
      _Inputs(),
      _FilledMask(),
      _OwnerMap(),
      _BoundaryFrontiers(),
      _ResultGray(),
      _FilledPixelCount(0),
      _HasCompleteMaskSet(!maskImages.empty() && maskImages.size() == _MosaicImages.size()) {
    _Inputs.reserve(_MosaicImages.size());
    for (size_t index = 0; index < _MosaicImages.size(); ++index) {
        InputBundle bundle = {};
        bundle.SourceImage = _MosaicImages[index];
        if (index < maskImages.size()) {
            bundle.Mask = maskImages[index];
        }
        bundle.OriginalIndex = index;
        _Inputs.emplace_back(std::move(bundle));
    }
}

void AdaptiveStripMosaicBase::Execute() {
    SuperDebug::ScopeTimer algorithmTimer(std::format("{} Execution", _AlgorithmName()));
    SuperDebug::Info("{} mosaic image size: {} x {}", _AlgorithmName(), AlgorithmResult.Height(), AlgorithmResult.Width());
    const int totalMosaicPixels = AlgorithmResult.Height() * AlgorithmResult.Width();

    if (_Inputs.empty()) {
        SuperDebug::Warn("{} received no input images.", _AlgorithmName());
        return;
    }

    if (!_HasCompleteMaskSet) {
        SuperDebug::Error("{} requires cloud masks with the same count as input images.", _AlgorithmName());
        SuperDebug::Error("Current behavior fallback: perform plain geographic mosaic.");
        _FallbackToPlainMosaic();
        return;
    }

    if (_ShouldSortInputsByLongitude()) {
        _SortInputsByLongitude();
    }

    SuperDebug::Info("{} preparing {} input image/mask pairs...", _AlgorithmName(), _Inputs.size());
    _PrepareInputs();
    _FilledMask = cv::Mat::zeros(AlgorithmResult.Height(), AlgorithmResult.Width(), CV_8UC1);
    _OwnerMap = cv::Mat(AlgorithmResult.Height(), AlgorithmResult.Width(), CV_32SC1, cv::Scalar(-1));
    _BoundaryFrontiers = {};
    _ResultGray = cv::Mat::zeros(AlgorithmResult.Height(), AlgorithmResult.Width(), CV_8UC1);
    _FilledPixelCount = 0;

    if (!_InitializeSeedRegion()) {
        SuperDebug::Warn("{} could not find a valid seed region. Fallback to plain geographic mosaic.", _AlgorithmName());
        _FallbackToPlainMosaic();
        return;
    }

    SuperDebug::Info("{} seed initialized. Filled pixels: {}/{}.", _AlgorithmName(), _FilledPixelCount, totalMosaicPixels);

    bool expandedInRound = false;
    int roundIndex = 0;
    do {
        ++roundIndex;
        expandedInRound = false;

        for (const auto direction : {ExpandDirection::Up, ExpandDirection::Right, ExpandDirection::Down, ExpandDirection::Left}) {
            const auto report = _ExpandDirection(direction);
            (void)report;
            if (report.Expanded()) {
                expandedInRound = true;
            }
        }

        SuperDebug::InfoInline("{} round {} completed. Progress={}, filled_pixels={}/{}, remaining_pixels={}.",
                               _AlgorithmName(), roundIndex, expandedInRound ? "yes" : "no",
                               _FilledPixelCount, totalMosaicPixels, totalMosaicPixels - _FilledPixelCount);
    } while (expandedInRound);

    SuperDebug::Info("{} completed. Final filled pixels: {}/{}.", _AlgorithmName(), _FilledPixelCount, totalMosaicPixels);
}

void AdaptiveStripMosaicBase::_PrepareInputs() {
    const auto maskPolicy = RSPIP::Algorithm::Detail::DefaultBinaryMaskPolicy();
    const bool hasMosaicGeoTransform = AlgorithmResult.GeoInfo.has_value() &&
                                       AlgorithmResult.GeoInfo->GeoTransform.size() >= 6 &&
                                       std::abs(AlgorithmResult.GeoInfo->GeoTransform[1]) > kCorrelationEpsilon &&
                                       std::abs(AlgorithmResult.GeoInfo->GeoTransform[5]) > kCorrelationEpsilon;
    const double mosaicPixelWidth = hasMosaicGeoTransform ? AlgorithmResult.GeoInfo->GeoTransform[1] : 1.0;
    const double mosaicPixelHeight = hasMosaicGeoTransform ? std::abs(AlgorithmResult.GeoInfo->GeoTransform[5]) : 1.0;
    const bool needGlobalStatistics = _NeedsGlobalValidStatistics();
    std::array<double, 3> globalValidSums = {0.0, 0.0, 0.0};
    std::array<double, 3> globalValidSquaredSums = {0.0, 0.0, 0.0};
    _HasGlobalValidStatistics = false;
    _GlobalValidPixelCount = 0;
    _GlobalValidMeans = {0.0, 0.0, 0.0};
    _GlobalValidStdDevs = {0.0, 0.0, 0.0};

    for (auto &input : _Inputs) {
        _OnInputBundlePrepared(input);
        input.IsEnabled = true;
        input.DisabledReason.clear();

        if (input.Mask.ImageData.empty()) {
            input.SelectionMask = cv::Mat::zeros(input.SourceImage.Height(), input.SourceImage.Width(), CV_8UC1);
        } else if (input.Mask.Height() != input.SourceImage.Height() || input.Mask.Width() != input.SourceImage.Width()) {
            input.SelectionMask = _CreateConstantMask(input.SourceImage, kFilledValue);
            input.IsEnabled = false;
            input.DisabledReason = "mask_size_mismatch";
            SuperDebug::Error(
                "{} input {} mask size mismatch. image={}x{}, mask={}x{}. The input will be disabled.",
                _AlgorithmName(),
                input.OriginalIndex,
                input.SourceImage.Width(),
                input.SourceImage.Height(),
                input.Mask.Width(),
                input.Mask.Height());
        } else {
            input.SelectionMask = BuildSelectionMask(input.Mask, maskPolicy);
            if (input.SelectionMask.empty()) {
                input.SelectionMask = _CreateConstantMask(input.SourceImage, kFilledValue);
                input.IsEnabled = false;
                input.DisabledReason = "mask_build_failed";
                SuperDebug::Error(
                    "{} input {} mask selection build failed. The input will be disabled.",
                    _AlgorithmName(),
                    input.OriginalIndex);
            }
        }
        input.ValidMask = cv::Mat::zeros(input.SourceImage.Height(), input.SourceImage.Width(), CV_8UC1);
        input.GrayImage = cv::Mat::zeros(input.SourceImage.Height(), input.SourceImage.Width(), CV_8UC1);

        if (hasMosaicGeoTransform && input.SourceImage.GeoInfo.has_value() && input.SourceImage.GeoInfo->GeoTransform.size() >= 6) {
            const bool sameResolution = std::abs(input.SourceImage.GeoInfo->GeoTransform[1] - AlgorithmResult.GeoInfo->GeoTransform[1]) <= kCorrelationEpsilon &&
                                        std::abs(std::abs(input.SourceImage.GeoInfo->GeoTransform[5]) - mosaicPixelHeight) <= kCorrelationEpsilon;
            const bool zeroRotation = std::abs(input.SourceImage.GeoInfo->GeoTransform[2]) <= kCorrelationEpsilon &&
                                      std::abs(input.SourceImage.GeoInfo->GeoTransform[4]) <= kCorrelationEpsilon &&
                                      std::abs(AlgorithmResult.GeoInfo->GeoTransform[2]) <= kCorrelationEpsilon &&
                                      std::abs(AlgorithmResult.GeoInfo->GeoTransform[4]) <= kCorrelationEpsilon;
            if (!sameResolution || !zeroRotation) {
                SuperDebug::Warn("{} input {} does not fully match the unified grid assumption.", _AlgorithmName(), input.OriginalIndex);
            }

            input.ColumnOffset = static_cast<int>(std::lround((input.SourceImage.GeoInfo->GeoTransform[0] - AlgorithmResult.GeoInfo->GeoTransform[0]) / mosaicPixelWidth));
            input.RowOffset = static_cast<int>(std::lround((AlgorithmResult.GeoInfo->GeoTransform[3] - input.SourceImage.GeoInfo->GeoTransform[3]) / mosaicPixelHeight));
        } else {
            input.ColumnOffset = 0;
            input.RowOffset = 0;
        }

        int clearPixels = 0;
        int extentPixels = 0;
        for (int row = 0; row < input.SourceImage.Height(); ++row) {
            const auto *imagePtr = input.SourceImage.ImageData.ptr<cv::Vec3b>(row);
            auto *validPtr = input.ValidMask.ptr<unsigned char>(row);
            auto *grayPtr = input.GrayImage.ptr<unsigned char>(row);

            for (int column = 0; column < input.SourceImage.Width(); ++column) {
                const auto &pixelValue = imagePtr[column];
                if (input.SourceImage.IsNoDataPixel(row, column)) {
                    continue;
                }

                grayPtr[column] = static_cast<unsigned char>(Util::BGRToGray(pixelValue));
                ++extentPixels;
                if (_GetSelectionMaskValue(input.SelectionMask, row, column) == 0) {
                    validPtr[column] = kFilledValue;
                    ++clearPixels;
                    if (needGlobalStatistics) {
                        ++_GlobalValidPixelCount;
                        for (size_t channelIndex = 0; channelIndex < globalValidSums.size(); ++channelIndex) {
                            const double channelValue = static_cast<double>(pixelValue[static_cast<int>(channelIndex)]);
                            globalValidSums[channelIndex] += channelValue;
                            globalValidSquaredSums[channelIndex] += channelValue * channelValue;
                        }
                    }
                }
            }
        }

        input.ClearRatio = extentPixels == 0 ? 0.0 : static_cast<double>(clearPixels) / static_cast<double>(extentPixels);
        if (input.IsEnabled) {
            SuperDebug::Info(
                "{} input {} prepared: extent_pixels={}, clear_pixels={}, clear_ratio={:.4f}, offset=({}, {}), status=enabled.",
                _AlgorithmName(), input.OriginalIndex, extentPixels, clearPixels, input.ClearRatio, input.RowOffset, input.ColumnOffset);
            if (clearPixels == 0) {
                SuperDebug::Warn(
                    "{} input {} has no clear pixels after mask filtering and will be skipped as a seed candidate.",
                    _AlgorithmName(),
                    input.OriginalIndex);
            }
        } else {
            SuperDebug::Warn(
                "{} input {} prepared: extent_pixels={}, clear_pixels={}, clear_ratio={:.4f}, offset=({}, {}), status=disabled, reason={}.",
                _AlgorithmName(), input.OriginalIndex, extentPixels, clearPixels, input.ClearRatio, input.RowOffset, input.ColumnOffset, input.DisabledReason);
        }
    }

    if (!needGlobalStatistics) {
        return;
    }

    _HasGlobalValidStatistics = _GlobalValidPixelCount > 0;
    if (_HasGlobalValidStatistics) {
        const double pixelCount = static_cast<double>(_GlobalValidPixelCount);
        for (size_t channelIndex = 0; channelIndex < _GlobalValidMeans.size(); ++channelIndex) {
            _GlobalValidMeans[channelIndex] = globalValidSums[channelIndex] / pixelCount;
            const double variance = std::max(0.0, globalValidSquaredSums[channelIndex] / pixelCount -
                                                      _GlobalValidMeans[channelIndex] * _GlobalValidMeans[channelIndex]);
            _GlobalValidStdDevs[channelIndex] = std::sqrt(variance);
        }

        SuperDebug::Info(
            "{} global valid statistics prepared: pixel_count={}, mean=({:.3f}, {:.3f}, {:.3f}), std=({:.3f}, {:.3f}, {:.3f}).",
            _AlgorithmName(), _GlobalValidPixelCount,
            _GlobalValidMeans[0], _GlobalValidMeans[1], _GlobalValidMeans[2],
            _GlobalValidStdDevs[0], _GlobalValidStdDevs[1], _GlobalValidStdDevs[2]);
    } else {
        SuperDebug::Warn("{} could not compute global valid statistics for strip balancing.", _AlgorithmName());
    }
}

void AdaptiveStripMosaicBase::_FallbackToPlainMosaic() {
    SuperDebug::Warn("{} fallback started: using plain geographic mosaic.", _AlgorithmName());
    for (const auto &imageData : _MosaicImages) {
        _PasteImageToMosaicResult(imageData);
    }
    SuperDebug::Warn("{} fallback completed.", _AlgorithmName());
}

void AdaptiveStripMosaicBase::_SortInputsByLongitude() {
    std::stable_sort(_Inputs.begin(), _Inputs.end(),
                     [](const InputBundle &left, const InputBundle &right) {
                         const auto leftLongitude = left.SourceImage.GeoInfo.has_value() ? left.SourceImage.GeoInfo->GetLongitude(0, 0) : std::numeric_limits<double>::max();
                         const auto rightLongitude = right.SourceImage.GeoInfo.has_value() ? right.SourceImage.GeoInfo->GetLongitude(0, 0) : std::numeric_limits<double>::max();
                         if (std::abs(leftLongitude - rightLongitude) > kCorrelationEpsilon) {
                             return leftLongitude < rightLongitude;
                         }
                         return left.OriginalIndex < right.OriginalIndex;
                     });
}

bool AdaptiveStripMosaicBase::_InitializeSeedRegion() {
    SeedRegion bestSeed = {};
    bool foundSeed = false;

    for (size_t index = 0; index < _Inputs.size(); ++index) {
        if (!_Inputs[index].IsEnabled) {
            SuperDebug::Warn(
                "{} seed candidate input {} skipped because it is disabled: {}.",
                _AlgorithmName(),
                _Inputs[index].OriginalIndex,
                _Inputs[index].DisabledReason);
            continue;
        }

        if (_Inputs[index].ValidMask.empty() || cv::countNonZero(_Inputs[index].ValidMask) == 0) {
            SuperDebug::Warn(
                "{} seed candidate input {} skipped because it has no valid clear pixels.",
                _AlgorithmName(),
                _Inputs[index].OriginalIndex);
            continue;
        }

        const auto rectangle = _FindLargestValidRectangle(_Inputs[index].ValidMask);
        const int mosaicRow = rectangle.y + _Inputs[index].RowOffset;
        const int mosaicColumn = rectangle.x + _Inputs[index].ColumnOffset;
        SuperDebug::Info(
            "{} seed candidate input {} ({}): clear_ratio={:.4f}, image_rect=(row={}, col={}, h={}, w={}), mosaic_rect=(row={}, col={}, h={}, w={}), area={}.",
            _AlgorithmName(), _Inputs[index].OriginalIndex, _Inputs[index].SourceImage.ImageName, _Inputs[index].ClearRatio,
            rectangle.y, rectangle.x, rectangle.height, rectangle.width,
            mosaicRow, mosaicColumn, rectangle.height, rectangle.width, rectangle.area());
        if (rectangle.area() <= 0) {
            continue;
        }

        if (!foundSeed || rectangle.area() > bestSeed.Rectangle.area()) {
            bestSeed.InputIndex = index;
            bestSeed.Rectangle = rectangle;
            bestSeed.ClearRatio = _Inputs[index].ClearRatio;
            foundSeed = true;
        }
    }

    if (!foundSeed || !bestSeed.IsValid()) {
        return false;
    }

    const auto &seedInput = _Inputs[bestSeed.InputIndex];
    const int selectedMosaicRow = bestSeed.Rectangle.y + seedInput.RowOffset;
    const int selectedMosaicColumn = bestSeed.Rectangle.x + seedInput.ColumnOffset;
    for (int row = bestSeed.Rectangle.y; row < bestSeed.Rectangle.y + bestSeed.Rectangle.height; ++row) {
        for (int column = bestSeed.Rectangle.x; column < bestSeed.Rectangle.x + bestSeed.Rectangle.width; ++column) {
            const int mosaicRow = row + seedInput.RowOffset;
            const int mosaicColumn = column + seedInput.ColumnOffset;
            if (mosaicRow < 0 || mosaicRow >= AlgorithmResult.Height() || mosaicColumn < 0 || mosaicColumn >= AlgorithmResult.Width()) {
                continue;
            }

            const auto pixelValue = seedInput.SourceImage.GetPixelValue<cv::Vec3b>(row, column);
            if (seedInput.SourceImage.IsNoDataPixel(row, column)) {
                continue;
            }

            _SetFilledPixel(mosaicRow, mosaicColumn, pixelValue, seedInput.GrayImage.at<unsigned char>(row, column), static_cast<int>(bestSeed.InputIndex));
        }
    }

    SuperDebug::Info(
        "{} seed selected by max_rect_area from input {} ({}): clear_ratio={:.4f}, image_rect=(row={}, col={}, h={}, w={}), mosaic_rect=(row={}, col={}, h={}, w={}), area={}.",
        _AlgorithmName(), seedInput.OriginalIndex, seedInput.SourceImage.ImageName, bestSeed.ClearRatio,
        bestSeed.Rectangle.y, bestSeed.Rectangle.x, bestSeed.Rectangle.height, bestSeed.Rectangle.width,
        selectedMosaicRow, selectedMosaicColumn, bestSeed.Rectangle.height, bestSeed.Rectangle.width,
        bestSeed.Rectangle.area());
    return true;
}

cv::Rect AdaptiveStripMosaicBase::_FindLargestValidRectangle(const cv::Mat &validMask) const {
    if (validMask.empty()) {
        return {};
    }

    std::vector<int> heights(validMask.cols, 0);
    cv::Rect bestRectangle = {};

    for (int row = 0; row < validMask.rows; ++row) {
        for (int column = 0; column < validMask.cols; ++column) {
            heights[column] = validMask.at<unsigned char>(row, column) == 0 ? 0 : heights[column] + 1;
        }

        std::stack<int> indexStack;
        for (int column = 0; column <= validMask.cols; ++column) {
            const int currentHeight = column == validMask.cols ? 0 : heights[column];
            while (!indexStack.empty() && currentHeight < heights[indexStack.top()]) {
                const int height = heights[indexStack.top()];
                indexStack.pop();
                const int leftBoundary = indexStack.empty() ? 0 : indexStack.top() + 1;
                const int width = column - leftBoundary;
                const cv::Rect candidateRectangle(leftBoundary, row - height + 1, width, height);
                if (_RectIsPreferred(candidateRectangle, bestRectangle)) {
                    bestRectangle = candidateRectangle;
                }
            }
            indexStack.push(column);
        }
    }

    return bestRectangle;
}

AdaptiveStripMosaicBase::ExpandPassReport AdaptiveStripMosaicBase::_ExpandDirection(ExpandDirection direction) {
    const auto collectStart = std::chrono::steady_clock::now();
    const auto segments = _CollectBoundarySegments(direction);
    const auto collectEnd = std::chrono::steady_clock::now();

    const auto [tangentRow, tangentColumn] = _GetTangentOffset(direction);
    ExpandPassReport report = {};
    report.SegmentCount = static_cast<int>(segments.size());
    report.BoundaryCollectMs = _ElapsedMilliseconds(collectStart, collectEnd);

    for (const auto &segment : segments) {
        int processedLength = 0;
        while (processedLength < segment.Length) {
            const int boundaryRow = segment.StartRow + tangentRow * processedLength;
            const int boundaryColumn = segment.StartColumn + tangentColumn * processedLength;

            const auto evalStart = std::chrono::steady_clock::now();
            const auto candidate = _SelectBestCandidate(direction, boundaryRow, boundaryColumn, segment.Length - processedLength);
            const auto evalEnd = std::chrono::steady_clock::now();
            report.CandidateEvalMs += _ElapsedMilliseconds(evalStart, evalEnd);

            if (!candidate.IsValid || candidate.Length <= 0) {
                ++report.FailedStartCount;
                ++processedLength;
                continue;
            }

            const auto applyStart = std::chrono::steady_clock::now();
            const auto applyResult = _ApplyCandidatePatch(candidate, direction, boundaryRow, boundaryColumn);
            const auto applyEnd = std::chrono::steady_clock::now();
            report.PatchApplyMs += _ElapsedMilliseconds(applyStart, applyEnd);

            if (applyResult == PatchApplyResult::Failed) {
                ++report.FailedStartCount;
                ++processedLength;
                continue;
            }

            report.WrittenPixelCount += candidate.Length * _StripWidth;
            ++report.SuccessPatchCount;
            processedLength += candidate.Length;
        }
    }

    return report;
}

std::vector<AdaptiveStripMosaicBase::BoundarySegment> AdaptiveStripMosaicBase::_CollectBoundarySegments(ExpandDirection direction) const {
    std::vector<BoundarySegment> segments = {};
    if (_FilledMask.empty()) {
        return segments;
    }

    const auto &frontier = _BoundaryFrontiers[_GetDirectionIndex(direction)];
    if (frontier.empty()) {
        return segments;
    }

    const bool horizontalSegments = direction == ExpandDirection::Up || direction == ExpandDirection::Down;
    BoundarySegment currentSegment = {};
    bool hasCurrentSegment = false;
    int previousRow = -1;
    int previousColumn = -1;

    for (const int key : frontier) {
        const auto [row, column] = _DecodeBoundaryKey(direction, key);
        const bool continueCurrentSegment = hasCurrentSegment &&
                                            ((horizontalSegments && row == previousRow && column == previousColumn + 1) ||
                                             (!horizontalSegments && column == previousColumn && row == previousRow + 1));
        if (!continueCurrentSegment) {
            if (hasCurrentSegment) {
                segments.push_back(currentSegment);
            }
            currentSegment = {row, column, 1};
            hasCurrentSegment = true;
        } else {
            ++currentSegment.Length;
        }

        previousRow = row;
        previousColumn = column;
    }

    if (hasCurrentSegment) {
        segments.push_back(currentSegment);
    }

    return segments;
}

bool AdaptiveStripMosaicBase::_IsBoundaryPixel(ExpandDirection direction, int row, int column) const {
    if (!_IsFilled(row, column)) {
        return false;
    }

    const auto [normalRow, normalColumn] = _GetNormalOffset(direction);
    return !_IsFilled(row + normalRow, column + normalColumn);
}

bool AdaptiveStripMosaicBase::_IsFilled(int row, int column) const {
    if (_FilledMask.empty() || row < 0 || row >= _FilledMask.rows || column < 0 || column >= _FilledMask.cols) {
        return false;
    }

    return _FilledMask.at<unsigned char>(row, column) == kFilledValue;
}

size_t AdaptiveStripMosaicBase::_GetDirectionIndex(ExpandDirection direction) const {
    switch (direction) {
    case ExpandDirection::Up:
        return 0;
    case ExpandDirection::Right:
        return 1;
    case ExpandDirection::Down:
        return 2;
    case ExpandDirection::Left:
        return 3;
    }

    return 0;
}

int AdaptiveStripMosaicBase::_EncodeBoundaryKey(ExpandDirection direction, int row, int column) const {
    if (direction == ExpandDirection::Up || direction == ExpandDirection::Down) {
        return row * AlgorithmResult.Width() + column;
    }

    return column * AlgorithmResult.Height() + row;
}

std::pair<int, int> AdaptiveStripMosaicBase::_DecodeBoundaryKey(ExpandDirection direction, int key) const {
    if (direction == ExpandDirection::Up || direction == ExpandDirection::Down) {
        return {key / AlgorithmResult.Width(), key % AlgorithmResult.Width()};
    }

    return {key % AlgorithmResult.Height(), key / AlgorithmResult.Height()};
}

void AdaptiveStripMosaicBase::_RefreshBoundaryPixel(ExpandDirection direction, int row, int column) {
    if (_FilledMask.empty() || row < 0 || row >= _FilledMask.rows || column < 0 || column >= _FilledMask.cols) {
        return;
    }

    auto &frontier = _BoundaryFrontiers[_GetDirectionIndex(direction)];
    const int key = _EncodeBoundaryKey(direction, row, column);
    if (_IsBoundaryPixel(direction, row, column)) {
        frontier.insert(key);
    } else {
        frontier.erase(key);
    }
}

void AdaptiveStripMosaicBase::_UpdateBoundaryFrontiersAfterFill(int row, int column) {
    for (const auto direction : {ExpandDirection::Up, ExpandDirection::Right, ExpandDirection::Down, ExpandDirection::Left}) {
        const auto [normalRow, normalColumn] = _GetNormalOffset(direction);
        _RefreshBoundaryPixel(direction, row, column);
        _RefreshBoundaryPixel(direction, row - normalRow, column - normalColumn);
    }
}

AdaptiveStripMosaicBase::CandidatePatch AdaptiveStripMosaicBase::_SelectBestCandidate(ExpandDirection direction, int boundaryRow, int boundaryColumn, int maxLength) const {
    if (!_OwnerMap.empty() && boundaryRow >= 0 && boundaryRow < _OwnerMap.rows && boundaryColumn >= 0 && boundaryColumn < _OwnerMap.cols) {
        const int ownerInputIndex = _OwnerMap.at<int>(boundaryRow, boundaryColumn);
        if (ownerInputIndex >= 0 && ownerInputIndex < static_cast<int>(_Inputs.size())) {
            const auto ownerCandidate = _EvaluateCandidate(static_cast<size_t>(ownerInputIndex), _Inputs[static_cast<size_t>(ownerInputIndex)],
                                                           direction, boundaryRow, boundaryColumn, maxLength);
            if (ownerCandidate.IsValid && ownerCandidate.Length > 0) {
                return ownerCandidate;
            }
        }
    }

    CandidatePatch bestCandidate = {};
    bestCandidate.Correlation = std::numeric_limits<double>::lowest();

    for (size_t inputIndex = 0; inputIndex < _Inputs.size(); ++inputIndex) {
        const auto candidate = _EvaluateCandidate(inputIndex, _Inputs[inputIndex], direction, boundaryRow, boundaryColumn, maxLength);
        if (!candidate.IsValid) {
            continue;
        }

        if (!bestCandidate.IsValid ||
            candidate.Correlation > bestCandidate.Correlation + kCorrelationEpsilon ||
            (std::abs(candidate.Correlation - bestCandidate.Correlation) <= kCorrelationEpsilon && candidate.Length > bestCandidate.Length) ||
            (std::abs(candidate.Correlation - bestCandidate.Correlation) <= kCorrelationEpsilon && candidate.Length == bestCandidate.Length &&
             _Inputs[candidate.InputIndex].OriginalIndex < _Inputs[bestCandidate.InputIndex].OriginalIndex)) {
            bestCandidate = candidate;
        }
    }

    return bestCandidate;
}

AdaptiveStripMosaicBase::CandidatePatch AdaptiveStripMosaicBase::_EvaluateCandidate(size_t inputIndex, const InputBundle &input, ExpandDirection direction, int boundaryRow, int boundaryColumn, int maxLength) const {
    CandidatePatch candidate = {};
    candidate.InputIndex = inputIndex;

    const auto [normalRow, normalColumn] = _GetNormalOffset(direction);
    const auto [tangentRow, tangentColumn] = _GetTangentOffset(direction);
    const int imageRows = input.SourceImage.Height();
    const int imageColumns = input.SourceImage.Width();

    double sumMosaic = 0.0;
    double sumInput = 0.0;
    double sumMosaicSq = 0.0;
    double sumInputSq = 0.0;
    double sumCross = 0.0;
    size_t sampleCount = 0;
    int validLength = 0;

    int currentBoundaryRow = boundaryRow;
    int currentBoundaryColumn = boundaryColumn;

    for (int step = 0; step < maxLength; ++step) {
        bool stepValid = true;
        for (int widthOffset = 0; widthOffset < _StripWidth; ++widthOffset) {
            const int mosaicInnerRow = currentBoundaryRow - normalRow * widthOffset;
            const int mosaicInnerColumn = currentBoundaryColumn - normalColumn * widthOffset;
            const int inputInnerRow = mosaicInnerRow - input.RowOffset;
            const int inputInnerColumn = mosaicInnerColumn - input.ColumnOffset;
            const int mosaicOuterRow = currentBoundaryRow + normalRow * (widthOffset + 1);
            const int mosaicOuterColumn = currentBoundaryColumn + normalColumn * (widthOffset + 1);
            const int inputOuterRow = mosaicOuterRow - input.RowOffset;
            const int inputOuterColumn = mosaicOuterColumn - input.ColumnOffset;

            if (mosaicInnerRow < 0 || mosaicInnerRow >= AlgorithmResult.Height() || mosaicInnerColumn < 0 || mosaicInnerColumn >= AlgorithmResult.Width() ||
                mosaicOuterRow < 0 || mosaicOuterRow >= AlgorithmResult.Height() || mosaicOuterColumn < 0 || mosaicOuterColumn >= AlgorithmResult.Width() ||
                inputInnerRow < 0 || inputInnerRow >= imageRows || inputInnerColumn < 0 || inputInnerColumn >= imageColumns ||
                inputOuterRow < 0 || inputOuterRow >= imageRows || inputOuterColumn < 0 || inputOuterColumn >= imageColumns ||
                !_IsFilled(mosaicInnerRow, mosaicInnerColumn) ||
                input.ValidMask.at<unsigned char>(inputInnerRow, inputInnerColumn) == 0 ||
                input.ValidMask.at<unsigned char>(inputOuterRow, inputOuterColumn) == 0) {
                stepValid = false;
                break;
            }

            const double mosaicGray = static_cast<double>(_ResultGray.at<unsigned char>(mosaicInnerRow, mosaicInnerColumn));
            const double inputGray = static_cast<double>(input.GrayImage.at<unsigned char>(inputInnerRow, inputInnerColumn));
            sumMosaic += mosaicGray;
            sumInput += inputGray;
            sumMosaicSq += mosaicGray * mosaicGray;
            sumInputSq += inputGray * inputGray;
            sumCross += mosaicGray * inputGray;
            ++sampleCount;
        }

        if (!stepValid) {
            break;
        }

        ++validLength;
        currentBoundaryRow += tangentRow;
        currentBoundaryColumn += tangentColumn;
    }

    if (validLength == 0 || sampleCount == 0) {
        return candidate;
    }

    candidate.Length = validLength;
    candidate.Correlation = _ComputePearsonCorrelation(sampleCount, sumMosaic, sumInput, sumMosaicSq, sumInputSq, sumCross);
    candidate.IsValid = true;
    return candidate;
}

void AdaptiveStripMosaicBase::_CopyPatchToResult(const InputBundle &input, ExpandDirection direction, int boundaryRow, int boundaryColumn, int length, size_t ownerInputIndex) {
    const auto [normalRow, normalColumn] = _GetNormalOffset(direction);
    const auto [tangentRow, tangentColumn] = _GetTangentOffset(direction);
    const int imageRows = input.SourceImage.Height();
    const int imageColumns = input.SourceImage.Width();

    int currentBoundaryRow = boundaryRow;
    int currentBoundaryColumn = boundaryColumn;

    for (int step = 0; step < length; ++step) {
        for (int widthOffset = 1; widthOffset <= _StripWidth; ++widthOffset) {
            const int targetMosaicRow = currentBoundaryRow + normalRow * widthOffset;
            const int targetMosaicColumn = currentBoundaryColumn + normalColumn * widthOffset;
            const int localRow = targetMosaicRow - input.RowOffset;
            const int localColumn = targetMosaicColumn - input.ColumnOffset;
            if (localRow < 0 || localRow >= imageRows || localColumn < 0 || localColumn >= imageColumns) {
                return;
            }

            _SetFilledPixel(targetMosaicRow,
                            targetMosaicColumn,
                            input.SourceImage.GetPixelValue<cv::Vec3b>(localRow, localColumn),
                            input.GrayImage.at<unsigned char>(localRow, localColumn),
                            static_cast<int>(ownerInputIndex));
        }

        currentBoundaryRow += tangentRow;
        currentBoundaryColumn += tangentColumn;
    }
}

bool AdaptiveStripMosaicBase::_BuildPatchPixels(const InputBundle &input, ExpandDirection direction, int boundaryRow, int boundaryColumn, int length,
                                                std::vector<PatchPixel> &patchPixels, bool &hasCompleteBoundary) const {
    patchPixels.clear();
    hasCompleteBoundary = true;

    const auto [normalRow, normalColumn] = _GetNormalOffset(direction);
    const auto [tangentRow, tangentColumn] = _GetTangentOffset(direction);
    const int imageRows = input.SourceImage.Height();
    const int imageColumns = input.SourceImage.Width();

    int currentBoundaryRow = boundaryRow;
    int currentBoundaryColumn = boundaryColumn;

    for (int step = 0; step < length; ++step) {
        for (int widthOffset = 1; widthOffset <= _StripWidth; ++widthOffset) {
            const int targetMosaicRow = currentBoundaryRow + normalRow * widthOffset;
            const int targetMosaicColumn = currentBoundaryColumn + normalColumn * widthOffset;
            const int boundaryMosaicRow = currentBoundaryRow - normalRow * (widthOffset - 1);
            const int boundaryMosaicColumn = currentBoundaryColumn - normalColumn * (widthOffset - 1);
            const int localRow = targetMosaicRow - input.RowOffset;
            const int localColumn = targetMosaicColumn - input.ColumnOffset;

            if (targetMosaicRow < 0 || targetMosaicRow >= AlgorithmResult.Height() || targetMosaicColumn < 0 || targetMosaicColumn >= AlgorithmResult.Width() ||
                localRow < 0 || localRow >= imageRows || localColumn < 0 || localColumn >= imageColumns) {
                return false;
            }

            PatchPixel patchPixel = {};
            patchPixel.MosaicRow = targetMosaicRow;
            patchPixel.MosaicColumn = targetMosaicColumn;
            patchPixel.Value = input.SourceImage.GetPixelValue<cv::Vec3b>(localRow, localColumn);
            patchPixel.GrayValue = input.GrayImage.at<unsigned char>(localRow, localColumn);

            if (boundaryMosaicRow < 0 || boundaryMosaicRow >= AlgorithmResult.Height() ||
                boundaryMosaicColumn < 0 || boundaryMosaicColumn >= AlgorithmResult.Width() ||
                !_IsFilled(boundaryMosaicRow, boundaryMosaicColumn)) {
                hasCompleteBoundary = false;
            } else {
                patchPixel.BoundaryValue = AlgorithmResult.GetPixelValue<cv::Vec3b>(boundaryMosaicRow, boundaryMosaicColumn);
            }

            patchPixels.emplace_back(patchPixel);
        }

        currentBoundaryRow += tangentRow;
        currentBoundaryColumn += tangentColumn;
    }

    return static_cast<int>(patchPixels.size()) == length * _StripWidth;
}

bool AdaptiveStripMosaicBase::_ComputeBalancedValues(const std::vector<PatchPixel> &patchPixels, std::vector<cv::Vec3b> &balancedValues) const {
    balancedValues.clear();
    if (patchPixels.empty() || !_HasGlobalValidStatistics) {
        return false;
    }

    try {
        _CandidateStripBuffer.create(1, static_cast<int>(patchPixels.size()), CV_8UC3);
        for (size_t index = 0; index < patchPixels.size(); ++index) {
            _CandidateStripBuffer.at<cv::Vec3b>(0, static_cast<int>(index)) = patchPixels[index].Value;
        }

        cv::cvtColor(_CandidateStripBuffer, _CandidateGrayBuffer, cv::COLOR_BGR2GRAY);
        cv::inRange(_CandidateGrayBuffer, 3, 210, _CandidateMaskBuffer);
        cv::inRange(_CandidateStripBuffer, cv::Scalar::all(0), cv::Scalar::all(0), _CandidateNoDataMaskBuffer);

        std::vector<double> targetMeans(_CandidateStripBuffer.channels(), 0.0);
        std::vector<double> targetStdDevs(_CandidateStripBuffer.channels(), 0.0);
        cv::meanStdDev(_CandidateStripBuffer, targetMeans, targetStdDevs, _CandidateMaskBuffer);

        std::vector<cv::Mat> targetChannels = {};
        cv::split(_CandidateStripBuffer, targetChannels);
        for (size_t channelIndex = 0; channelIndex < targetChannels.size(); ++channelIndex) {
            auto &targetChannel = targetChannels[channelIndex];
            const auto alpha = (targetStdDevs[channelIndex] < 1e-6) ? 1.0 : (_GlobalValidStdDevs[channelIndex] / targetStdDevs[channelIndex]);
            const auto beta = _GlobalValidMeans[channelIndex] - alpha * targetMeans[channelIndex];
            targetChannel.convertTo(targetChannel, -1, alpha, beta);
        }

        cv::merge(targetChannels, _BalancedStripBuffer);
        _BalancedStripBuffer.setTo(0, _CandidateNoDataMaskBuffer);
        if (_BalancedStripBuffer.empty() || _BalancedStripBuffer.cols != _CandidateStripBuffer.cols) {
            return false;
        }

        balancedValues.reserve(patchPixels.size());
        for (int column = 0; column < _BalancedStripBuffer.cols; ++column) {
            balancedValues.emplace_back(_BalancedStripBuffer.at<cv::Vec3b>(0, column));
        }
        return true;
    } catch (const cv::Exception &exception) {
        SuperDebug::Warn("{} strip color balance failed and will keep original strip values: {}", _AlgorithmName(), exception.what());
        return false;
    } catch (const std::exception &exception) {
        SuperDebug::Warn("{} strip color balance failed and will keep original strip values: {}", _AlgorithmName(), exception.what());
        return false;
    } catch (...) {
        SuperDebug::Warn("{} strip color balance failed with an unknown error and will keep original strip values.", _AlgorithmName());
        return false;
    }
}

bool AdaptiveStripMosaicBase::_BalancePatchPixelsInPlace(std::vector<PatchPixel> &patchPixels) const {
    std::vector<cv::Vec3b> balancedValues = {};
    if (!_ComputeBalancedValues(patchPixels, balancedValues) || balancedValues.size() != patchPixels.size()) {
        return false;
    }

    for (size_t index = 0; index < patchPixels.size(); ++index) {
        patchPixels[index].Value = balancedValues[index];
        patchPixels[index].GrayValue = static_cast<unsigned char>(Util::BGRToGray(patchPixels[index].Value));
    }
    return true;
}

void AdaptiveStripMosaicBase::_PastePatchDirectly(const std::vector<PatchPixel> &patchPixels, size_t ownerInputIndex) {
    for (const auto &patchPixel : patchPixels) {
        _SetFilledPixel(patchPixel.MosaicRow, patchPixel.MosaicColumn, patchPixel.Value, patchPixel.GrayValue, static_cast<int>(ownerInputIndex));
    }
}

std::pair<int, int> AdaptiveStripMosaicBase::_GetNormalOffset(ExpandDirection direction) const {
    switch (direction) {
    case ExpandDirection::Up:
        return {-1, 0};
    case ExpandDirection::Right:
        return {0, 1};
    case ExpandDirection::Down:
        return {1, 0};
    case ExpandDirection::Left:
        return {0, -1};
    }

    return {0, 0};
}

std::pair<int, int> AdaptiveStripMosaicBase::_GetTangentOffset(ExpandDirection direction) const {
    switch (direction) {
    case ExpandDirection::Up:
    case ExpandDirection::Down:
        return {0, 1};
    case ExpandDirection::Right:
    case ExpandDirection::Left:
        return {1, 0};
    }

    return {0, 0};
}

void AdaptiveStripMosaicBase::_SetFilledPixel(int mosaicRow, int mosaicColumn, const cv::Vec3b &pixelValue, unsigned char grayValue, int ownerInputIndex) {
    if (mosaicRow < 0 || mosaicRow >= AlgorithmResult.Height() || mosaicColumn < 0 || mosaicColumn >= AlgorithmResult.Width()) {
        return;
    }

    const bool wasFilled = _FilledMask.at<unsigned char>(mosaicRow, mosaicColumn) == kFilledValue;
    if (!wasFilled) {
        ++_FilledPixelCount;
    }
    AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
    _FilledMask.at<unsigned char>(mosaicRow, mosaicColumn) = kFilledValue;
    if (!_OwnerMap.empty()) {
        _OwnerMap.at<int>(mosaicRow, mosaicColumn) = ownerInputIndex;
    }
    _ResultGray.at<unsigned char>(mosaicRow, mosaicColumn) = grayValue;
    if (!wasFilled) {
        _UpdateBoundaryFrontiersAfterFill(mosaicRow, mosaicColumn);
    }
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm::Detail
