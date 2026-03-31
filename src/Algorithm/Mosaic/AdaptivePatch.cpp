#include "Algorithm/Mosaic/AdaptivePatch.h"
#include "Util/General.h"
#include "Util/SuperDebug.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
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

} // namespace

AdaptivePatch::AdaptivePatch(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks)
    : MosaicAlgorithmBase(imageDatas),
      _Inputs(),
      _FilledMask(),
      _OwnerMap(),
      _ResultGray(),
      _FilledPixelCount(0),
      _LegacyGeoMapCallCount(0),
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

void AdaptivePatch::Execute() {
    SuperDebug::ScopeTimer algorithmTimer("AdaptivePatch Execution");
    SuperDebug::Info("Mosaic Image Size: {} x {}", AlgorithmResult.Height(), AlgorithmResult.Width());
    const int totalMosaicPixels = AlgorithmResult.Height() * AlgorithmResult.Width();

    if (_Inputs.empty()) {
        SuperDebug::Warn("AdaptivePatch received no input images.");
        return;
    }

    if (_Inputs.size() != _MosaicImages.size()) {
        SuperDebug::Error("AdaptivePatch input initialization failed. Fallback to plain geographic mosaic.");
        _FallbackToPlainMosaic();
        return;
    }

    if (!_HasCompleteMaskSet) {
        SuperDebug::Error("AdaptivePatch requires cloud masks with the same count as input images.");
        SuperDebug::Error("Current behavior fallback: perform plain geographic mosaic without adaptive patch selection.");
        _FallbackToPlainMosaic();
        return;
    }

    SuperDebug::Info("AdaptivePatch preparing {} input image/mask pairs...", _Inputs.size());
    _PrepareInputs();
    _FilledMask = cv::Mat::zeros(AlgorithmResult.Height(), AlgorithmResult.Width(), CV_8UC1);
    _OwnerMap = cv::Mat(AlgorithmResult.Height(), AlgorithmResult.Width(), CV_32SC1, cv::Scalar(-1));
    _ResultGray = cv::Mat::zeros(AlgorithmResult.Height(), AlgorithmResult.Width(), CV_8UC1);
    _FilledPixelCount = 0;
    _LegacyGeoMapCallCount = 0;

    if (!_InitializeSeedRegion()) {
        SuperDebug::Warn("AdaptivePatch could not find a valid seed region. Fallback to plain geographic mosaic.");
        _FallbackToPlainMosaic();
        return;
    }
    SuperDebug::Info("AdaptivePatch seed initialized. Filled pixels: {}/{}.", _FilledPixelCount, totalMosaicPixels);

    bool expandedInRound = false;
    int roundIndex = 0;
    do {
        ++roundIndex;
        expandedInRound = false;

        for (const auto direction : {ExpandDirection::Up, ExpandDirection::Right, ExpandDirection::Down, ExpandDirection::Left}) {
            const auto report = _ExpandDirection(direction);
            // SuperDebug::Info(
            //     "AdaptivePatch round {} [{}] pass finished: segments={}, strips={}, skipped_starts={}, written_pixels={}, boundary_ms={:.3f}, eval_ms={:.3f}, copy_ms={:.3f}, filled={}/{}.",
            //     roundIndex, _GetDirectionName(direction), report.SegmentCount, report.SuccessStripCount,
            //     report.FailedStartCount, report.WrittenPixelCount, report.BoundaryCollectMs,
            //     report.CandidateEvalMs, report.PatchCopyMs, _FilledPixelCount, totalMosaicPixels);
            if (report.Expanded()) {
                expandedInRound = true;
            }
        }

        SuperDebug::InfoInline("AdaptivePatch round {} completed. Progress={}, filled_pixels={}/{}, remaining_pixels={}.",
                               roundIndex, expandedInRound ? "yes" : "no", _FilledPixelCount, totalMosaicPixels, totalMosaicPixels - _FilledPixelCount);
    } while (expandedInRound);

    SuperDebug::Info("AdaptivePatch completed. Final filled pixels: {}/{}, legacy_geo_map_calls={}.",
                     _FilledPixelCount, totalMosaicPixels, _LegacyGeoMapCallCount);
}

void AdaptivePatch::_PrepareInputs() {
    const bool hasMosaicGeoTransform = AlgorithmResult.GeoTransform.size() >= 6 &&
                                       std::abs(AlgorithmResult.GeoTransform[1]) > kCorrelationEpsilon &&
                                       std::abs(AlgorithmResult.GeoTransform[5]) > kCorrelationEpsilon;
    const double mosaicPixelWidth = hasMosaicGeoTransform ? AlgorithmResult.GeoTransform[1] : 1.0;
    const double mosaicPixelHeight = hasMosaicGeoTransform ? std::abs(AlgorithmResult.GeoTransform[5]) : 1.0;

    for (auto &input : _Inputs) {
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
                SuperDebug::Warn("AdaptivePatch input {} does not fully match the unified grid assumption.", input.OriginalIndex);
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
                }
            }
        }

        input.ClearRatio = extentPixels == 0 ? 0.0 : static_cast<double>(clearPixels) / static_cast<double>(extentPixels);
        SuperDebug::Info(
            "AdaptivePatch input {} prepared: extent_pixels={}, clear_pixels={}, clear_ratio={:.4f}, offset=({}, {}).",
            input.OriginalIndex, extentPixels, clearPixels, input.ClearRatio, input.RowOffset, input.ColumnOffset);
    }
}

void AdaptivePatch::_FallbackToPlainMosaic() {
    SuperDebug::Warn("AdaptivePatch fallback started: using plain geographic mosaic.");
    for (const auto &imageData : _MosaicImages) {
        _PasteImageToMosaicResult(imageData);
    }
    SuperDebug::Warn("AdaptivePatch fallback completed.");
}

bool AdaptivePatch::_InitializeSeedRegion() {
    SeedRegion bestSeed = {};
    bool foundSeed = false;

    for (size_t index = 0; index < _Inputs.size(); ++index) {
        const auto rectangle = _FindLargestValidRectangle(_Inputs[index].ValidMask);
        const int mosaicRow = rectangle.y + _Inputs[index].RowOffset;
        const int mosaicColumn = rectangle.x + _Inputs[index].ColumnOffset;
        SuperDebug::Info(
            "AdaptivePatch seed candidate input {} ({}): clear_ratio={:.4f}, image_rect=(row={}, col={}, h={}, w={}), mosaic_rect=(row={}, col={}, h={}, w={}), area={}.",
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
        "AdaptivePatch seed selected by max_rect_area from input {} ({}): clear_ratio={:.4f}, image_rect=(row={}, col={}, h={}, w={}), mosaic_rect=(row={}, col={}, h={}, w={}), area={}.",
        seedInput.OriginalIndex, seedInput.Image.ImageName, bestSeed.ClearRatio,
        bestSeed.Rectangle.y, bestSeed.Rectangle.x, bestSeed.Rectangle.height, bestSeed.Rectangle.width,
        selectedMosaicRow, selectedMosaicColumn, bestSeed.Rectangle.height, bestSeed.Rectangle.width,
        bestSeed.Rectangle.area());
    return true;
}

cv::Rect AdaptivePatch::_FindLargestValidRectangle(const cv::Mat &validMask) const {
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

AdaptivePatch::ExpandPassReport AdaptivePatch::_ExpandDirection(ExpandDirection direction) {
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

            const auto copyStart = std::chrono::steady_clock::now();
            _CopyPatchToResult(_Inputs[candidate.InputIndex], direction, boundaryRow, boundaryColumn, candidate.Length, candidate.InputIndex);
            const auto copyEnd = std::chrono::steady_clock::now();
            report.PatchCopyMs += _ElapsedMilliseconds(copyStart, copyEnd);

            report.WrittenPixelCount += candidate.Length * _StripWidth;
            ++report.SuccessStripCount;
            processedLength += candidate.Length;
        }
    }

    return report;
}

std::vector<AdaptivePatch::BoundarySegment> AdaptivePatch::_CollectBoundarySegments(ExpandDirection direction) const {
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

bool AdaptivePatch::_IsBoundaryPixel(ExpandDirection direction, int row, int column) const {
    if (!_IsFilled(row, column)) {
        return false;
    }

    const auto [normalRow, normalColumn] = _GetNormalOffset(direction);
    return !_IsFilled(row + normalRow, column + normalColumn);
}

bool AdaptivePatch::_IsFilled(int row, int column) const {
    if (_FilledMask.empty() || row < 0 || row >= _FilledMask.rows || column < 0 || column >= _FilledMask.cols) {
        return false;
    }

    return _FilledMask.at<unsigned char>(row, column) == kFilledValue;
}

AdaptivePatch::CandidatePatch AdaptivePatch::_SelectBestCandidate(ExpandDirection direction, int boundaryRow, int boundaryColumn, int maxLength) const {
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

AdaptivePatch::CandidatePatch AdaptivePatch::_EvaluateCandidate(size_t inputIndex, const InputBundle &input, ExpandDirection direction, int boundaryRow, int boundaryColumn, int maxLength) const {
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
            const int inputOuterRow = currentBoundaryRow + normalRow * (widthOffset + 1) - input.RowOffset;
            const int inputOuterColumn = currentBoundaryColumn + normalColumn * (widthOffset + 1) - input.ColumnOffset;

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

void AdaptivePatch::_CopyPatchToResult(const InputBundle &input, ExpandDirection direction, int boundaryRow, int boundaryColumn, int length, size_t ownerInputIndex) {
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
            const int localRow = targetMosaicRow - input.RowOffset;
            const int localColumn = targetMosaicColumn - input.ColumnOffset;
            if (localRow < 0 || localRow >= imageRows || localColumn < 0 || localColumn >= imageColumns) {
                return;
            }

            _SetFilledPixel(
                targetMosaicRow,
                targetMosaicColumn,
                input.Image.GetPixelValue<cv::Vec3b>(localRow, localColumn),
                input.GrayImage.at<unsigned char>(localRow, localColumn),
                static_cast<int>(ownerInputIndex));
        }

        currentBoundaryRow += tangentRow;
        currentBoundaryColumn += tangentColumn;
    }
}

std::pair<int, int> AdaptivePatch::_GetNormalOffset(ExpandDirection direction) const {
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

std::pair<int, int> AdaptivePatch::_GetTangentOffset(ExpandDirection direction) const {
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

const char *AdaptivePatch::_GetDirectionName(ExpandDirection direction) const {
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

void AdaptivePatch::_SetFilledPixel(int mosaicRow, int mosaicColumn, const cv::Vec3b &pixelValue, unsigned char grayValue, int ownerInputIndex) {
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
