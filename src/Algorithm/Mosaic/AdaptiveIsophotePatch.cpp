#include "Algorithm/Mosaic/AdaptiveIsophotePatch.h"
#include "Algorithm/Reconstruct/IsophoteConstrain.h"
#include "Util/General.h"
#include "Util/SuperDebug.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <stack>

namespace RSPIP::Algorithm::MosaicAlgorithm {

namespace {

constexpr unsigned char kFilledValue = 255;
constexpr double kCorrelationEpsilon = 1e-9;

unsigned char _GetCloudMaskValue(const CloudMask &cloudMask, int row, int column) {
    if (cloudMask.ImageData.empty() || row < 0 || row >= cloudMask.Height() || column < 0 || column >= cloudMask.Width()) {
        return kFilledValue;
    }

    switch (cloudMask.GetBandCounts()) {
    case 1:
        return cloudMask.GetPixelValue<unsigned char>(row, column);
    case 3:
        return cloudMask.GetPixelValue<cv::Vec3b>(row, column)[0];
    case 4:
        return cloudMask.GetPixelValue<cv::Vec4b>(row, column)[0];
    default:
        return kFilledValue;
    }
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

ImageGeoBounds _BuildImageBounds(const GeoImage &image) {
    if (image.ImageData.empty() || image.GeoTransform.size() < 6) {
        return image.ImageBounds;
    }

    ImageGeoBounds bounds = {
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::lowest()};

    const auto updateBounds = [&](int row, int column) {
        const auto [latitude, longitude] = image.GetLatLon(row, column);
        bounds.MinLongitude = std::min(bounds.MinLongitude, longitude);
        bounds.MaxLongitude = std::max(bounds.MaxLongitude, longitude);
        bounds.MinLatitude = std::min(bounds.MinLatitude, latitude);
        bounds.MaxLatitude = std::max(bounds.MaxLatitude, latitude);
    };

    const int lastRow = std::max(0, image.Height() - 1);
    const int lastColumn = std::max(0, image.Width() - 1);
    updateBounds(0, 0);
    updateBounds(0, lastColumn);
    updateBounds(lastRow, 0);
    updateBounds(lastRow, lastColumn);
    return bounds;
}

} // namespace

AdaptiveIsophotePatch::AdaptiveIsophotePatch(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks)
    : MosaicAlgorithmBase(imageDatas),
      _Inputs(),
      _FilledMask(),
      _OwnerMap(),
      _ResultGray(),
      _FilledPixelCount(0),
      _HasCompleteMaskSet(!cloudMasks.empty() && cloudMasks.size() == imageDatas.size()) {
    _Inputs.reserve(imageDatas.size());
    for (size_t index = 0; index < imageDatas.size(); ++index) {
        InputBundle bundle = {};
        bundle.Image = imageDatas[index];
        if (index < cloudMasks.size()) {
            bundle.Mask = cloudMasks[index];
        }
        bundle.OriginalIndex = index;
        _Inputs.emplace_back(std::move(bundle));
    }
}

void AdaptiveIsophotePatch::Execute() {
    SuperDebug::ScopeTimer algorithmTimer("AdaptiveIsophotePatch Execution");
    SuperDebug::Info("Mosaic Image Size: {} x {}", AlgorithmResult.Height(), AlgorithmResult.Width());
    const int totalMosaicPixels = AlgorithmResult.Height() * AlgorithmResult.Width();

    if (_Inputs.empty()) {
        SuperDebug::Warn("AdaptiveIsophotePatch received no input images.");
        return;
    }

    if (!_HasCompleteMaskSet) {
        SuperDebug::Error("AdaptiveIsophotePatch requires cloud masks with the same count as input images.");
        SuperDebug::Error("Current behavior fallback: perform plain geographic mosaic without isophote-guided patching.");
        _FallbackToPlainMosaic();
        return;
    }

    _SortInputsByLongitude();
    _PrepareInputs();
    _FilledMask = cv::Mat::zeros(AlgorithmResult.Height(), AlgorithmResult.Width(), CV_8UC1);
    _OwnerMap = cv::Mat(AlgorithmResult.Height(), AlgorithmResult.Width(), CV_32SC1, cv::Scalar(-1));
    _ResultGray = cv::Mat::zeros(AlgorithmResult.Height(), AlgorithmResult.Width(), CV_8UC1);
    _FilledPixelCount = 0;

    if (!_InitializeSeedRegion()) {
        SuperDebug::Warn("AdaptiveIsophotePatch could not find a valid seed region. Fallback to plain geographic mosaic.");
        _FallbackToPlainMosaic();
        return;
    }

    SuperDebug::Info("AdaptiveIsophotePatch seed initialized. Filled pixels: {}/{}.", _FilledPixelCount, totalMosaicPixels);

    bool expandedInRound = false;
    int roundIndex = 0;
    do {
        ++roundIndex;
        expandedInRound = false;

        for (const auto direction : {ExpandDirection::Up, ExpandDirection::Right, ExpandDirection::Down, ExpandDirection::Left}) {
            const auto report = _ExpandDirection(direction);
            // SuperDebug::Info(
            //     "AdaptiveIsophotePatch round {} [{}] pass finished: segments={}, strips={}, reconstructed={}, direct_paste_fallback={}, skipped_starts={}, written_pixels={}, boundary_ms={:.3f}, eval_ms={:.3f}, apply_ms={:.3f}, filled={}/{}.",
            //     roundIndex, _GetDirectionName(direction), report.SegmentCount, report.SuccessStripCount,
            //     report.ReconstructedStripCount, report.DirectPasteStripCount, report.FailedStartCount,
            //     report.WrittenPixelCount, report.BoundaryCollectMs, report.CandidateEvalMs,
            //     report.PatchApplyMs, _FilledPixelCount, totalMosaicPixels);
            if (report.Expanded()) {
                expandedInRound = true;
            }
        }

        SuperDebug::InfoInline("AdaptiveIsophotePatch round {} completed. Progress={}, filled_pixels={}/{}, remaining_pixels={}.",
                               roundIndex, expandedInRound ? "yes" : "no", _FilledPixelCount, totalMosaicPixels, totalMosaicPixels - _FilledPixelCount);
    } while (expandedInRound);

    SuperDebug::Info("AdaptiveIsophotePatch completed. Final filled pixels: {}/{}.", _FilledPixelCount, totalMosaicPixels);
}

void AdaptiveIsophotePatch::_SortInputsByLongitude() {
    std::stable_sort(_Inputs.begin(), _Inputs.end(),
                     [](const InputBundle &left, const InputBundle &right) {
                         const auto leftLongitude = left.Image.GetLongitude(0, 0);
                         const auto rightLongitude = right.Image.GetLongitude(0, 0);
                         if (std::abs(leftLongitude - rightLongitude) > kCorrelationEpsilon) {
                             return leftLongitude < rightLongitude;
                         }
                         return left.OriginalIndex < right.OriginalIndex;
                     });
}

void AdaptiveIsophotePatch::_PrepareInputs() {
    const bool hasMosaicGeoTransform = AlgorithmResult.GeoTransform.size() >= 6 &&
                                       std::abs(AlgorithmResult.GeoTransform[1]) > kCorrelationEpsilon &&
                                       std::abs(AlgorithmResult.GeoTransform[5]) > kCorrelationEpsilon;
    const double mosaicPixelWidth = hasMosaicGeoTransform ? AlgorithmResult.GeoTransform[1] : 1.0;
    const double mosaicPixelHeight = hasMosaicGeoTransform ? std::abs(AlgorithmResult.GeoTransform[5]) : 1.0;
    std::array<double, 3> globalValidSums = {0.0, 0.0, 0.0};
    std::array<double, 3> globalValidSquaredSums = {0.0, 0.0, 0.0};
    _HasGlobalValidStatistics = false;
    _GlobalValidPixelCount = 0;
    _GlobalValidMeans = {0.0, 0.0, 0.0};
    _GlobalValidStdDevs = {0.0, 0.0, 0.0};

    for (auto &input : _Inputs) {
        input.Mask.SetSourceImage(input.Image);
        input.ValidMask = cv::Mat::zeros(input.Image.Height(), input.Image.Width(), CV_8UC1);
        input.GrayImage = cv::Mat::zeros(input.Image.Height(), input.Image.Width(), CV_8UC1);

        if (hasMosaicGeoTransform && input.Image.GeoTransform.size() >= 6) {
            const bool sameResolution = std::abs(input.Image.GeoTransform[1] - AlgorithmResult.GeoTransform[1]) <= kCorrelationEpsilon &&
                                        std::abs(std::abs(input.Image.GeoTransform[5]) - mosaicPixelHeight) <= kCorrelationEpsilon;
            const bool zeroRotation = std::abs(input.Image.GeoTransform[2]) <= kCorrelationEpsilon &&
                                      std::abs(input.Image.GeoTransform[4]) <= kCorrelationEpsilon &&
                                      std::abs(AlgorithmResult.GeoTransform[2]) <= kCorrelationEpsilon &&
                                      std::abs(AlgorithmResult.GeoTransform[4]) <= kCorrelationEpsilon;
            if (!sameResolution || !zeroRotation) {
                SuperDebug::Warn("AdaptiveIsophotePatch input {} does not fully match the unified grid assumption.", input.OriginalIndex);
            }

            input.ColumnOffset = static_cast<int>(std::lround((input.Image.GeoTransform[0] - AlgorithmResult.GeoTransform[0]) / mosaicPixelWidth));
            input.RowOffset = static_cast<int>(std::lround((AlgorithmResult.GeoTransform[3] - input.Image.GeoTransform[3]) / mosaicPixelHeight));
        } else {
            input.ColumnOffset = 0;
            input.RowOffset = 0;
        }

        int clearPixels = 0;
        int extentPixels = 0;
        for (int row = 0; row < input.Image.Height(); ++row) {
            const auto *imagePtr = input.Image.ImageData.ptr<cv::Vec3b>(row);
            auto *validPtr = input.ValidMask.ptr<unsigned char>(row);
            auto *grayPtr = input.GrayImage.ptr<unsigned char>(row);

            for (int column = 0; column < input.Image.Width(); ++column) {
                const auto &pixelValue = imagePtr[column];
                if (pixelValue == input.Image.NonData) {
                    continue;
                }

                grayPtr[column] = static_cast<unsigned char>(Util::BGRToGray(pixelValue));
                ++extentPixels;
                if (_GetCloudMaskValue(input.Mask, row, column) == input.Mask.NonData[0]) {
                    validPtr[column] = kFilledValue;
                    ++clearPixels;
                    ++_GlobalValidPixelCount;
                    for (size_t channelIndex = 0; channelIndex < globalValidSums.size(); ++channelIndex) {
                        const double channelValue = static_cast<double>(pixelValue[static_cast<int>(channelIndex)]);
                        globalValidSums[channelIndex] += channelValue;
                        globalValidSquaredSums[channelIndex] += channelValue * channelValue;
                    }
                }
            }
        }

        input.ClearRatio = extentPixels == 0 ? 0.0 : static_cast<double>(clearPixels) / static_cast<double>(extentPixels);
        SuperDebug::Info(
            "AdaptiveIsophotePatch input {} prepared: extent_pixels={}, clear_pixels={}, clear_ratio={:.4f}, offset=({}, {}).",
            input.OriginalIndex, extentPixels, clearPixels, input.ClearRatio, input.RowOffset, input.ColumnOffset);
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
            "AdaptiveIsophotePatch global valid statistics prepared: pixel_count={}, mean=({:.3f}, {:.3f}, {:.3f}), std=({:.3f}, {:.3f}, {:.3f}).",
            _GlobalValidPixelCount,
            _GlobalValidMeans[0], _GlobalValidMeans[1], _GlobalValidMeans[2],
            _GlobalValidStdDevs[0], _GlobalValidStdDevs[1], _GlobalValidStdDevs[2]);
    } else {
        SuperDebug::Warn("AdaptiveIsophotePatch could not compute global valid statistics for seed color balancing.");
    }
}

void AdaptiveIsophotePatch::_FallbackToPlainMosaic() {
    SuperDebug::Warn("AdaptiveIsophotePatch fallback started: using plain geographic mosaic.");
    for (const auto &imageData : _MosaicImages) {
        _PasteImageToMosaicResult(imageData);
    }
    SuperDebug::Warn("AdaptiveIsophotePatch fallback completed.");
}

bool AdaptiveIsophotePatch::_InitializeSeedRegion() {
    SeedRegion bestSeed = {};
    bool foundSeed = false;

    for (size_t index = 0; index < _Inputs.size(); ++index) {
        const auto rectangle = _FindLargestValidRectangle(_Inputs[index].ValidMask);
        const int mosaicRow = rectangle.y + _Inputs[index].RowOffset;
        const int mosaicColumn = rectangle.x + _Inputs[index].ColumnOffset;
        SuperDebug::Info(
            "AdaptiveIsophotePatch seed candidate input {} ({}): clear_ratio={:.4f}, image_rect=(row={}, col={}, h={}, w={}), mosaic_rect=(row={}, col={}, h={}, w={}), area={}.",
            _Inputs[index].OriginalIndex, _Inputs[index].Image.ImageName, _Inputs[index].ClearRatio,
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

            const auto pixelValue = seedInput.Image.GetPixelValue<cv::Vec3b>(row, column);
            if (pixelValue == seedInput.Image.NonData) {
                continue;
            }

            _SetFilledPixel(mosaicRow, mosaicColumn, pixelValue, seedInput.GrayImage.at<unsigned char>(row, column), static_cast<int>(bestSeed.InputIndex));
        }
    }

    SuperDebug::Info(
        "AdaptiveIsophotePatch seed selected by max_rect_area from input {} ({}): clear_ratio={:.4f}, image_rect=(row={}, col={}, h={}, w={}), mosaic_rect=(row={}, col={}, h={}, w={}), area={}.",
        seedInput.OriginalIndex, seedInput.Image.ImageName, bestSeed.ClearRatio,
        bestSeed.Rectangle.y, bestSeed.Rectangle.x, bestSeed.Rectangle.height, bestSeed.Rectangle.width,
        selectedMosaicRow, selectedMosaicColumn, bestSeed.Rectangle.height, bestSeed.Rectangle.width,
        bestSeed.Rectangle.area());
    return true;
}

cv::Rect AdaptiveIsophotePatch::_FindLargestValidRectangle(const cv::Mat &validMask) const {
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

AdaptiveIsophotePatch::ExpandPassReport AdaptiveIsophotePatch::_ExpandDirection(ExpandDirection direction) {
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
            ++report.SuccessStripCount;
            if (applyResult == PatchApplyResult::Reconstructed) {
                ++report.ReconstructedStripCount;
            } else {
                ++report.DirectPasteStripCount;
            }
            processedLength += candidate.Length;
        }
    }

    return report;
}

std::vector<AdaptiveIsophotePatch::BoundarySegment> AdaptiveIsophotePatch::_CollectBoundarySegments(ExpandDirection direction) const {
    std::vector<BoundarySegment> segments = {};
    if (_FilledMask.empty()) {
        return segments;
    }

    if (direction == ExpandDirection::Up || direction == ExpandDirection::Down) {
        for (int row = 0; row < _FilledMask.rows; ++row) {
            int column = 0;
            while (column < _FilledMask.cols) {
                if (!_IsBoundaryPixel(direction, row, column)) {
                    ++column;
                    continue;
                }

                const int startColumn = column;
                while (column < _FilledMask.cols && _IsBoundaryPixel(direction, row, column)) {
                    ++column;
                }
                segments.push_back({row, startColumn, column - startColumn});
            }
        }
        return segments;
    }

    for (int column = 0; column < _FilledMask.cols; ++column) {
        int row = 0;
        while (row < _FilledMask.rows) {
            if (!_IsBoundaryPixel(direction, row, column)) {
                ++row;
                continue;
            }

            const int startRow = row;
            while (row < _FilledMask.rows && _IsBoundaryPixel(direction, row, column)) {
                ++row;
            }
            segments.push_back({startRow, column, row - startRow});
        }
    }

    return segments;
}

bool AdaptiveIsophotePatch::_IsBoundaryPixel(ExpandDirection direction, int row, int column) const {
    if (!_IsFilled(row, column)) {
        return false;
    }

    const auto [normalRow, normalColumn] = _GetNormalOffset(direction);
    return !_IsFilled(row + normalRow, column + normalColumn);
}

bool AdaptiveIsophotePatch::_IsFilled(int row, int column) const {
    if (_FilledMask.empty() || row < 0 || row >= _FilledMask.rows || column < 0 || column >= _FilledMask.cols) {
        return false;
    }

    return _FilledMask.at<unsigned char>(row, column) == kFilledValue;
}

AdaptiveIsophotePatch::CandidatePatch AdaptiveIsophotePatch::_SelectBestCandidate(ExpandDirection direction, int boundaryRow, int boundaryColumn, int maxLength) const {
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

AdaptiveIsophotePatch::CandidatePatch AdaptiveIsophotePatch::_EvaluateCandidate(size_t inputIndex, const InputBundle &input, ExpandDirection direction, int boundaryRow, int boundaryColumn, int maxLength) const {
    CandidatePatch candidate = {};
    candidate.InputIndex = inputIndex;

    const auto [normalRow, normalColumn] = _GetNormalOffset(direction);
    const auto [tangentRow, tangentColumn] = _GetTangentOffset(direction);
    const int imageRows = input.Image.Height();
    const int imageColumns = input.Image.Width();

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

AdaptiveIsophotePatch::PatchApplyResult AdaptiveIsophotePatch::_ApplyCandidatePatch(const CandidatePatch &candidate, ExpandDirection direction, int boundaryRow, int boundaryColumn) {
    if (!candidate.IsValid || candidate.InputIndex >= _Inputs.size() || candidate.Length <= 0) {
        return PatchApplyResult::Failed;
    }

    std::vector<PatchPixel> patchPixels = {};
    bool hasCompleteBoundary = false;
    if (!_BuildPatchPixels(_Inputs[candidate.InputIndex], direction, boundaryRow, boundaryColumn, candidate.Length, patchPixels, hasCompleteBoundary) || patchPixels.empty()) {
        return PatchApplyResult::Failed;
    }

    std::vector<PatchPixel> balancedPatchPixels = patchPixels;
    const bool hasBalancedFallback = hasCompleteBoundary && _BalancePatchPixels(balancedPatchPixels);

    if (_ReconstructPatchLocally(patchPixels, candidate.InputIndex)) {
        return PatchApplyResult::Reconstructed;
    }

    if (hasBalancedFallback) {
        _PastePatchDirectly(balancedPatchPixels, candidate.InputIndex);
    } else {
        _PastePatchDirectly(patchPixels, candidate.InputIndex);
    }
    return PatchApplyResult::DirectPaste;
}

bool AdaptiveIsophotePatch::_BuildPatchPixels(const InputBundle &input, ExpandDirection direction, int boundaryRow, int boundaryColumn, int length,
                                              std::vector<PatchPixel> &patchPixels, bool &hasCompleteBoundary) const {
    patchPixels.clear();
    hasCompleteBoundary = true;

    const auto [normalRow, normalColumn] = _GetNormalOffset(direction);
    const auto [tangentRow, tangentColumn] = _GetTangentOffset(direction);
    const int imageRows = input.Image.Height();
    const int imageColumns = input.Image.Width();

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

            if (targetMosaicRow < 0 || targetMosaicRow >= AlgorithmResult.Height() || targetMosaicColumn < 0 || targetMosaicColumn >= AlgorithmResult.Width()) {
                return false;
            }

            if (localRow < 0 || localRow >= imageRows || localColumn < 0 || localColumn >= imageColumns) {
                return false;
            }

            PatchPixel patchPixel = {};
            patchPixel.MosaicRow = targetMosaicRow;
            patchPixel.MosaicColumn = targetMosaicColumn;
            patchPixel.Value = input.Image.GetPixelValue<cv::Vec3b>(localRow, localColumn);
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

bool AdaptiveIsophotePatch::_BalancePatchPixels(std::vector<PatchPixel> &patchPixels) const {
    if (patchPixels.empty() || !_HasGlobalValidStatistics) {
        return false;
    }

    try {
        cv::Mat candidateStrip(1, static_cast<int>(patchPixels.size()), CV_8UC3);

        for (size_t index = 0; index < patchPixels.size(); ++index) {
            candidateStrip.at<cv::Vec3b>(0, static_cast<int>(index)) = patchPixels[index].Value;
        }

        cv::Mat targetMaskMat, targetGrayMat;
        cv::cvtColor(candidateStrip, targetGrayMat, cv::COLOR_BGR2GRAY);
        cv::inRange(targetGrayMat, 3, 210, targetMaskMat);
        cv::Mat targetNoDataMask;
        cv::inRange(candidateStrip, cv::Scalar::all(0), cv::Scalar::all(0), targetNoDataMask);

        std::vector<double> targetMeans(candidateStrip.channels(), 0.0);
        std::vector<double> targetStdDevs(candidateStrip.channels(), 0.0);
        cv::meanStdDev(candidateStrip, targetMeans, targetStdDevs, targetMaskMat);

        std::vector<cv::Mat> targetChannels = {};
        cv::split(candidateStrip, targetChannels);
        for (size_t channelIndex = 0; channelIndex < targetChannels.size(); ++channelIndex) {
            auto &targetChannel = targetChannels[channelIndex];
            const auto alpha = (targetStdDevs[channelIndex] < 1e-6) ? 1.0 : (_GlobalValidStdDevs[channelIndex] / targetStdDevs[channelIndex]);
            const auto beta = _GlobalValidMeans[channelIndex] - alpha * targetMeans[channelIndex];
            targetChannel.convertTo(targetChannel, -1, alpha, beta);
        }

        cv::Mat balancedStrip = {};
        cv::merge(targetChannels, balancedStrip);
        balancedStrip.setTo(0, targetNoDataMask);
        if (balancedStrip.empty() || balancedStrip.cols != candidateStrip.cols) {
            return false;
        }

        for (int column = 0; column < balancedStrip.cols; ++column) {
            patchPixels[static_cast<size_t>(column)].Value = balancedStrip.at<cv::Vec3b>(0, column);
            patchPixels[static_cast<size_t>(column)].GrayValue =
                static_cast<unsigned char>(Util::BGRToGray(patchPixels[static_cast<size_t>(column)].Value));
        }
        return true;
    } catch (const cv::Exception &exception) {
        SuperDebug::Warn("AdaptiveIsophotePatch strip color balance failed and will keep original strip values: {}", exception.what());
        return false;
    } catch (const std::exception &exception) {
        SuperDebug::Warn("AdaptiveIsophotePatch strip color balance failed and will keep original strip values: {}", exception.what());
        return false;
    } catch (...) {
        SuperDebug::Warn("AdaptiveIsophotePatch strip color balance failed with an unknown error and will keep original strip values.");
        return false;
    }
}

bool AdaptiveIsophotePatch::_ReconstructPatchLocally(const std::vector<PatchPixel> &patchPixels, size_t ownerInputIndex) {
    if (patchPixels.empty()) {
        return false;
    }

    int minRow = std::numeric_limits<int>::max();
    int maxRow = std::numeric_limits<int>::lowest();
    int minColumn = std::numeric_limits<int>::max();
    int maxColumn = std::numeric_limits<int>::lowest();
    for (const auto &patchPixel : patchPixels) {
        minRow = std::min(minRow, patchPixel.MosaicRow);
        maxRow = std::max(maxRow, patchPixel.MosaicRow);
        minColumn = std::min(minColumn, patchPixel.MosaicColumn);
        maxColumn = std::max(maxColumn, patchPixel.MosaicColumn);
    }

    const int windowTop = std::max(0, minRow - 1);
    const int windowBottom = std::min(AlgorithmResult.Height() - 1, maxRow + 1);
    const int windowLeft = std::max(0, minColumn - 1);
    const int windowRight = std::min(AlgorithmResult.Width() - 1, maxColumn + 1);
    const cv::Rect window(windowLeft, windowTop, windowRight - windowLeft + 1, windowBottom - windowTop + 1);
    if (window.width <= 0 || window.height <= 0) {
        return false;
    }

    auto localReconstructImage = _CreateLocalGeoImage(window);
    if (localReconstructImage.ImageData.empty()) {
        return false;
    }

    auto localReferImage = localReconstructImage;
    cv::Mat localMaskData = cv::Mat::zeros(window.height, window.width, CV_8UC1);

    for (const auto &patchPixel : patchPixels) {
        const int localRow = patchPixel.MosaicRow - window.y;
        const int localColumn = patchPixel.MosaicColumn - window.x;
        if (localRow < 0 || localRow >= window.height || localColumn < 0 || localColumn >= window.width) {
            return false;
        }

        localMaskData.at<unsigned char>(localRow, localColumn) = kFilledValue;
        localReconstructImage.SetPixelValue(localRow, localColumn, localReconstructImage.NonData);
        localReferImage.SetPixelValue(localRow, localColumn, patchPixel.Value);
    }

    CloudMask localMask(localMaskData, localReconstructImage);
    localMask.SetSourceImage(localReconstructImage);

    try {
        ReconstructAlgorithm::IsophoteConstrain reconstructAlgorithm(localReconstructImage, localReferImage, localMask);
        reconstructAlgorithm.SetMaxIterations(_MaxIterations);
        reconstructAlgorithm.SetEpsilon(_Epsilon);
        reconstructAlgorithm.Execute();

        for (const auto &patchPixel : patchPixels) {
            const int localRow = patchPixel.MosaicRow - window.y;
            const int localColumn = patchPixel.MosaicColumn - window.x;
            const auto reconstructedPixel = reconstructAlgorithm.AlgorithmResult.GetPixelValue<cv::Vec3b>(localRow, localColumn);
            _SetFilledPixel(
                patchPixel.MosaicRow,
                patchPixel.MosaicColumn,
                reconstructedPixel,
                static_cast<unsigned char>(Util::BGRToGray(reconstructedPixel)),
                static_cast<int>(ownerInputIndex));
        }
        return true;
    } catch (const std::exception &exception) {
        SuperDebug::Warn("AdaptiveIsophotePatch local reconstruction failed and will fallback to color-balanced direct paste when available: {}", exception.what());
        return false;
    } catch (...) {
        SuperDebug::Warn("AdaptiveIsophotePatch local reconstruction failed with an unknown error and will fallback to color-balanced direct paste when available.");
        return false;
    }
}

void AdaptiveIsophotePatch::_PastePatchDirectly(const std::vector<PatchPixel> &patchPixels, size_t ownerInputIndex) {
    for (const auto &patchPixel : patchPixels) {
        _SetFilledPixel(patchPixel.MosaicRow, patchPixel.MosaicColumn, patchPixel.Value, patchPixel.GrayValue, static_cast<int>(ownerInputIndex));
    }
}

GeoImage AdaptiveIsophotePatch::_CreateLocalGeoImage(const cv::Rect &window) const {
    GeoImage localImage = AlgorithmResult;
    localImage.ImageData = AlgorithmResult.ImageData(window).clone();
    localImage.NonData = AlgorithmResult.NonData;
    localImage.Projection = AlgorithmResult.Projection;

    if (localImage.GeoTransform.size() >= 6) {
        localImage.GeoTransform[0] = AlgorithmResult.GeoTransform[0] + window.x * AlgorithmResult.GeoTransform[1] + window.y * AlgorithmResult.GeoTransform[2];
        localImage.GeoTransform[3] = AlgorithmResult.GeoTransform[3] + window.x * AlgorithmResult.GeoTransform[4] + window.y * AlgorithmResult.GeoTransform[5];
        localImage.ImageBounds = _BuildImageBounds(localImage);
    }

    return localImage;
}

std::pair<int, int> AdaptiveIsophotePatch::_GetNormalOffset(ExpandDirection direction) const {
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

std::pair<int, int> AdaptiveIsophotePatch::_GetTangentOffset(ExpandDirection direction) const {
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

const char *AdaptiveIsophotePatch::_GetDirectionName(ExpandDirection direction) const {
    switch (direction) {
    case ExpandDirection::Up:
        return "Up";
    case ExpandDirection::Right:
        return "Right";
    case ExpandDirection::Down:
        return "Down";
    case ExpandDirection::Left:
        return "Left";
    }

    return "Unknown";
}

void AdaptiveIsophotePatch::_SetFilledPixel(int mosaicRow, int mosaicColumn, const cv::Vec3b &pixelValue, unsigned char grayValue, int ownerInputIndex) {
    if (mosaicRow < 0 || mosaicRow >= AlgorithmResult.Height() || mosaicColumn < 0 || mosaicColumn >= AlgorithmResult.Width()) {
        return;
    }

    if (_FilledMask.at<unsigned char>(mosaicRow, mosaicColumn) != kFilledValue) {
        ++_FilledPixelCount;
    }

    AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
    _FilledMask.at<unsigned char>(mosaicRow, mosaicColumn) = kFilledValue;
    if (!_OwnerMap.empty()) {
        _OwnerMap.at<int>(mosaicRow, mosaicColumn) = ownerInputIndex;
    }
    _ResultGray.at<unsigned char>(mosaicRow, mosaicColumn) = grayValue;
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm
