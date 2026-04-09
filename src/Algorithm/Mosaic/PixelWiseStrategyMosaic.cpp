#include "Algorithm/Mosaic/PixelWiseStrategyMosaic.h"
#include "Algorithm/Detail/AlgorithmValidation.h"
#include "Algorithm/Detail/MaskPolicies.h"
#include "Basic/RegionExtraction.h"
#include "Util/Color.h"
#include "Util/SuperDebug.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>

namespace RSPIP::Algorithm::MosaicAlgorithm {

namespace {

constexpr auto kAlgorithmName = "PixelWiseStrategyMosaic";

unsigned char _RoundAndClampToByte(double value) {
    const long roundedValue = std::lround(value);
    return static_cast<unsigned char>(std::clamp(roundedValue, 0L, 255L));
}

std::string_view _StrategyName(PixelWiseOverlapStrategy strategy) {
    switch (strategy) {
    case PixelWiseOverlapStrategy::LastWriteWins:
        return "LastWriteWins";
    case PixelWiseOverlapStrategy::HighlightOverlapRed:
        return "HighlightOverlapRed";
    case PixelWiseOverlapStrategy::MeanOfValidPixels:
        return "MeanOfValidPixels";
    case PixelWiseOverlapStrategy::MedianOfValidPixels:
        return "MedianOfValidPixels";
    }

    return "Unknown";
}

} // namespace

PixelWiseStrategyMosaic::PixelWiseStrategyMosaic(std::vector<Image> imageDatas)
    : MosaicAlgorithmBase(std::move(imageDatas)), _MaskImages(), _OverlapStrategy(PixelWiseOverlapStrategy::LastWriteWins) {}

PixelWiseStrategyMosaic::PixelWiseStrategyMosaic(std::vector<Image> imageDatas, std::vector<Image> maskImages)
    : MosaicAlgorithmBase(std::move(imageDatas)), _MaskImages(std::move(maskImages)), _OverlapStrategy(PixelWiseOverlapStrategy::LastWriteWins) {}

void PixelWiseStrategyMosaic::SetOverlapStrategy(PixelWiseOverlapStrategy strategy) {
    _OverlapStrategy = strategy;
}

void PixelWiseStrategyMosaic::Execute() {
    if (!_ValidateExecuteInputs()) {
        Detail::ResetImageResult(AlgorithmResult);
        return;
    }

    SuperDebug::Info("{} mosaic image size: {} x {}", kAlgorithmName, AlgorithmResult.Height(), AlgorithmResult.Width());
    SuperDebug::Info("{} overlap strategy: {}", kAlgorithmName, _StrategyName(_OverlapStrategy));

    switch (_OverlapStrategy) {
    case PixelWiseOverlapStrategy::LastWriteWins:
        if (!_MaskImages.empty()) {
            SuperDebug::Warn("{} strategy {} ignores the provided mask inputs.", kAlgorithmName, _StrategyName(_OverlapStrategy));
        }
        _ExecuteLastWriteWins();
        break;
    case PixelWiseOverlapStrategy::HighlightOverlapRed:
        if (!_MaskImages.empty()) {
            SuperDebug::Warn("{} strategy {} ignores the provided mask inputs.", kAlgorithmName, _StrategyName(_OverlapStrategy));
        }
        _ExecuteHighlightOverlapRed();
        break;
    case PixelWiseOverlapStrategy::MeanOfValidPixels:
        if (!_ValidateAggregateStrategyInputs()) {
            Detail::ResetImageResult(AlgorithmResult);
            return;
        }

        {
            std::vector<cv::Mat> selectionMasks = {};
            if (!_BuildAggregateSelectionMasks(selectionMasks)) {
                Detail::ResetImageResult(AlgorithmResult);
                return;
            }

            _ExecuteMeanOfValidPixels(selectionMasks);
        }
        break;
    case PixelWiseOverlapStrategy::MedianOfValidPixels:
        if (!_ValidateAggregateStrategyInputs()) {
            Detail::ResetImageResult(AlgorithmResult);
            return;
        }

        {
            std::vector<cv::Mat> selectionMasks = {};
            if (!_BuildAggregateSelectionMasks(selectionMasks)) {
                Detail::ResetImageResult(AlgorithmResult);
                return;
            }

            if (_MaskImages.empty()) {
                SuperDebug::Warn("{} strategy {} is running without mask inputs and will exclude NoData only.",
                                 kAlgorithmName,
                                 _StrategyName(_OverlapStrategy));
            }

            _ExecuteMedianOfValidPixels(selectionMasks);
        }
        break;
    default:
        SuperDebug::Warn("{} received an unsupported overlap strategy. Falling back to LastWriteWins.", kAlgorithmName);
        if (!_MaskImages.empty()) {
            SuperDebug::Warn("{} strategy {} ignores the provided mask inputs.", kAlgorithmName, _StrategyName(PixelWiseOverlapStrategy::LastWriteWins));
        }
        _ExecuteLastWriteWins();
        break;
    }

    SuperDebug::Info("{} completed.", kAlgorithmName);
}

bool PixelWiseStrategyMosaic::_ValidateExecuteInputs() const {
    if (_MosaicImages.empty() || AlgorithmResult.ImageData.empty() || !AlgorithmResult.GeoInfo.has_value()) {
        SuperDebug::Error("{} could not execute because mosaic initialization is invalid.", kAlgorithmName);
        return false;
    }

    return true;
}

bool PixelWiseStrategyMosaic::_ValidateAggregateStrategyInputs() const {
    if (_MaskImages.empty()) {
        return true;
    }

    if (_MaskImages.size() != _MosaicImages.size()) {
        SuperDebug::Error("{} requires mask count to match input image count. images={}, masks={}.",
                          kAlgorithmName,
                          _MosaicImages.size(),
                          _MaskImages.size());
        return false;
    }

    for (size_t index = 0; index < _MaskImages.size(); ++index) {
        if (_MaskImages[index].ImageData.empty()) {
            SuperDebug::Error("{} requires a non-empty mask image at index {}.", kAlgorithmName, index);
            return false;
        }

        if (_MaskImages[index].Height() != _MosaicImages[index].Height() || _MaskImages[index].Width() != _MosaicImages[index].Width()) {
            SuperDebug::Error("{} requires mask {} to match image {} size. image={}x{}, mask={}x{}.",
                              kAlgorithmName,
                              index,
                              index,
                              _MosaicImages[index].Width(),
                              _MosaicImages[index].Height(),
                              _MaskImages[index].Width(),
                              _MaskImages[index].Height());
            return false;
        }
    }

    return true;
}

bool PixelWiseStrategyMosaic::_BuildAggregateSelectionMasks(std::vector<cv::Mat> &selectionMasks) const {
    selectionMasks.clear();
    if (_MaskImages.empty()) {
        return true;
    }

    const auto maskPolicy = Detail::DefaultBinaryMaskPolicy();
    selectionMasks.reserve(_MaskImages.size());
    for (size_t index = 0; index < _MaskImages.size(); ++index) {
        cv::Mat selectionMask = BuildSelectionMask(_MaskImages[index], maskPolicy);
        if (selectionMask.empty()) {
            SuperDebug::Error("{} failed to build a selection mask for input {}.", kAlgorithmName, index);
            return false;
        }

        selectionMasks.emplace_back(std::move(selectionMask));
    }

    return true;
}

bool PixelWiseStrategyMosaic::_TryProjectPixel(const Image &imageData, int row, int column, const cv::Mat *selectionMask,
                                               int &mosaicRow, int &mosaicColumn, cv::Vec3b &pixelValue) const {
    if (!imageData.GeoInfo.has_value() || !AlgorithmResult.GeoInfo.has_value()) {
        return false;
    }

    if (imageData.IsNoDataPixel(row, column)) {
        return false;
    }

    if (selectionMask != nullptr && selectionMask->at<unsigned char>(row, column) != 0) {
        return false;
    }

    const auto [latitude, longitude] = imageData.GeoInfo->GetLatLon(row, column);
    const auto [targetRow, targetColumn] = AlgorithmResult.GeoInfo->LatLonToRC(latitude, longitude);
    if (targetRow == -1 || targetColumn == -1) {
        return false;
    }

    mosaicRow = targetRow;
    mosaicColumn = targetColumn;
    pixelValue = imageData.GetPixelValue<cv::Vec3b>(row, column);
    return true;
}

void PixelWiseStrategyMosaic::_ExecuteLastWriteWins() {
    AlgorithmResult.ImageData.setTo(AlgorithmResult.GetFillScalarForOpenCV());

    for (const auto &imageData : _MosaicImages) {
        for (int row = 0; row < imageData.Height(); ++row) {
            for (int column = 0; column < imageData.Width(); ++column) {
                int mosaicRow = -1;
                int mosaicColumn = -1;
                cv::Vec3b pixelValue = {};
                if (!_TryProjectPixel(imageData, row, column, nullptr, mosaicRow, mosaicColumn, pixelValue)) {
                    continue;
                }

                AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
            }
        }
    }
}

void PixelWiseStrategyMosaic::_ExecuteHighlightOverlapRed() {
    AlgorithmResult.ImageData.setTo(AlgorithmResult.GetFillScalarForOpenCV());

    for (const auto &imageData : _MosaicImages) {
        for (int row = 0; row < imageData.Height(); ++row) {
            for (int column = 0; column < imageData.Width(); ++column) {
                int mosaicRow = -1;
                int mosaicColumn = -1;
                cv::Vec3b pixelValue = {};
                if (!_TryProjectPixel(imageData, row, column, nullptr, mosaicRow, mosaicColumn, pixelValue)) {
                    continue;
                }

                if (AlgorithmResult.IsNoDataPixel(mosaicRow, mosaicColumn)) {
                    AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
                } else {
                    AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, Color::Red);
                }
            }
        }
    }
}

void PixelWiseStrategyMosaic::_ExecuteMeanOfValidPixels(const std::vector<cv::Mat> &selectionMasks) {
    AlgorithmResult.ImageData.setTo(AlgorithmResult.GetFillScalarForOpenCV());

    cv::Mat sumBuffer = cv::Mat::zeros(AlgorithmResult.Height(), AlgorithmResult.Width(), CV_64FC3);
    cv::Mat countBuffer = cv::Mat::zeros(AlgorithmResult.Height(), AlgorithmResult.Width(), CV_32SC1);

    for (size_t index = 0; index < _MosaicImages.size(); ++index) {
        const cv::Mat *selectionMask = selectionMasks.empty() ? nullptr : &selectionMasks[index];
        _AccumulateImageToBuffers(_MosaicImages[index], selectionMask, sumBuffer, countBuffer);
    }

    _ResolveMeanResult(sumBuffer, countBuffer);
}

void PixelWiseStrategyMosaic::_ExecuteMedianOfValidPixels(const std::vector<cv::Mat> &selectionMasks) {
    AlgorithmResult.ImageData.setTo(AlgorithmResult.GetFillScalarForOpenCV());

    std::vector<std::vector<cv::Vec3b>> sampleBuckets(static_cast<size_t>(AlgorithmResult.Height() * AlgorithmResult.Width()));
    for (size_t index = 0; index < _MosaicImages.size(); ++index) {
        const cv::Mat *selectionMask = selectionMasks.empty() ? nullptr : &selectionMasks[index];
        _AccumulateImageToSampleBuckets(_MosaicImages[index], selectionMask, sampleBuckets);
    }

    _ResolveMedianResult(sampleBuckets);
}

void PixelWiseStrategyMosaic::_AccumulateImageToBuffers(const Image &imageData, const cv::Mat *selectionMask, cv::Mat &sumBuffer, cv::Mat &countBuffer) const {
    for (int row = 0; row < imageData.Height(); ++row) {
        for (int column = 0; column < imageData.Width(); ++column) {
            int mosaicRow = -1;
            int mosaicColumn = -1;
            cv::Vec3b pixelValue = {};
            if (!_TryProjectPixel(imageData, row, column, selectionMask, mosaicRow, mosaicColumn, pixelValue)) {
                continue;
            }

            auto &sumValue = sumBuffer.at<cv::Vec3d>(mosaicRow, mosaicColumn);
            sumValue[0] += static_cast<double>(pixelValue[0]);
            sumValue[1] += static_cast<double>(pixelValue[1]);
            sumValue[2] += static_cast<double>(pixelValue[2]);
            ++countBuffer.at<int>(mosaicRow, mosaicColumn);
        }
    }
}

void PixelWiseStrategyMosaic::_AccumulateImageToSampleBuckets(const Image &imageData, const cv::Mat *selectionMask,
                                                              std::vector<std::vector<cv::Vec3b>> &sampleBuckets) const {
    for (int row = 0; row < imageData.Height(); ++row) {
        for (int column = 0; column < imageData.Width(); ++column) {
            int mosaicRow = -1;
            int mosaicColumn = -1;
            cv::Vec3b pixelValue = {};
            if (!_TryProjectPixel(imageData, row, column, selectionMask, mosaicRow, mosaicColumn, pixelValue)) {
                continue;
            }

            const size_t bucketIndex = static_cast<size_t>(mosaicRow * AlgorithmResult.Width() + mosaicColumn);
            sampleBuckets[bucketIndex].push_back(pixelValue);
        }
    }
}

void PixelWiseStrategyMosaic::_ResolveMeanResult(const cv::Mat &sumBuffer, const cv::Mat &countBuffer) {
    int filledPixelCount = 0;

    for (int row = 0; row < AlgorithmResult.Height(); ++row) {
        const auto *sumPtr = sumBuffer.ptr<cv::Vec3d>(row);
        const auto *countPtr = countBuffer.ptr<int>(row);
        auto *resultPtr = AlgorithmResult.ImageData.ptr<cv::Vec3b>(row);

        for (int column = 0; column < AlgorithmResult.Width(); ++column) {
            if (countPtr[column] <= 0) {
                continue;
            }

            const double divisor = static_cast<double>(countPtr[column]);
            resultPtr[column] = cv::Vec3b(
                _RoundAndClampToByte(sumPtr[column][0] / divisor),
                _RoundAndClampToByte(sumPtr[column][1] / divisor),
                _RoundAndClampToByte(sumPtr[column][2] / divisor));
            ++filledPixelCount;
        }
    }

    SuperDebug::Info("{} filled {} pixels with valid overlap samples.", kAlgorithmName, filledPixelCount);
}

void PixelWiseStrategyMosaic::_ResolveMedianResult(const std::vector<std::vector<cv::Vec3b>> &sampleBuckets) {
    int filledPixelCount = 0;

    for (int row = 0; row < AlgorithmResult.Height(); ++row) {
        auto *resultPtr = AlgorithmResult.ImageData.ptr<cv::Vec3b>(row);
        for (int column = 0; column < AlgorithmResult.Width(); ++column) {
            const auto &samples = sampleBuckets[static_cast<size_t>(row * AlgorithmResult.Width() + column)];
            if (samples.empty()) {
                continue;
            }

            std::array<std::vector<unsigned char>, 3> channelValues = {};
            for (auto &channel : channelValues) {
                channel.reserve(samples.size());
            }

            for (const auto &sample : samples) {
                channelValues[0].push_back(sample[0]);
                channelValues[1].push_back(sample[1]);
                channelValues[2].push_back(sample[2]);
            }

            cv::Vec3b medianPixel = {};
            const size_t medianIndex = (samples.size() - 1) / 2;
            for (size_t channelIndex = 0; channelIndex < channelValues.size(); ++channelIndex) {
                auto &values = channelValues[channelIndex];
                std::nth_element(values.begin(), values.begin() + static_cast<std::ptrdiff_t>(medianIndex), values.end());
                medianPixel[static_cast<int>(channelIndex)] = values[medianIndex];
            }

            resultPtr[column] = medianPixel;
            ++filledPixelCount;
        }
    }

    SuperDebug::Info("{} filled {} pixels with valid overlap samples.", kAlgorithmName, filledPixelCount);
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm
