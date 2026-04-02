#include "Algorithm/ColorBalance/MatchStatistic.h"
#include "Algorithm/Detail/AlgorithmValidation.h"
#include "Algorithm/Detail/MaskPolicies.h"
#include "Basic/RegionExtraction.h"
#include "Util/SuperDebug.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace RSPIP::Algorithm::ColorBalanceAlgorithm {

MatchStatistics::MatchStatistics(Image targetImage, Image referenceImage, Image mask)
    : ColorBalanceAlgorithmBase(std::move(targetImage), std::move(referenceImage)), _Mask(std::move(mask)) {}

void MatchStatistics::Execute() {
    if (!Detail::ValidateNonEmpty(_TargetImage, "MatchStatistics", "target image") ||
        !Detail::ValidateNonEmpty(_ReferenceImage, "MatchStatistics", "reference image") ||
        !Detail::ValidateBgrImage(_TargetImage, "MatchStatistics", "target image") ||
        !Detail::ValidateBgrImage(_ReferenceImage, "MatchStatistics", "reference image")) {
        Detail::ResetImageResult(AlgorithmResult);
        return;
    }

    cv::Mat targetMaskMat;
    cv::Mat targetGreyMat;
    cv::cvtColor(_TargetImage.ImageData, targetGreyMat, cv::COLOR_BGR2GRAY);
    cv::inRange(targetGreyMat, 3, 210, targetMaskMat);
    cv::Mat targetNoDataMask = cv::Mat::zeros(_TargetImage.Height(), _TargetImage.Width(), CV_8UC1);
    if (_TargetImage.HasNoData()) {
        for (int row = 0; row < _TargetImage.Height(); ++row) {
            auto *maskPtr = targetNoDataMask.ptr<unsigned char>(row);
            for (int column = 0; column < _TargetImage.Width(); ++column) {
                maskPtr[column] = _TargetImage.IsNoDataPixel(row, column) ? 255 : 0;
            }
        }
    }

    cv::Mat referenceMaskMat;
    cv::Mat referenceGreyMat;
    cv::cvtColor(_ReferenceImage.ImageData, referenceGreyMat, cv::COLOR_BGR2GRAY);
    cv::inRange(referenceGreyMat, 3, 210, referenceMaskMat);

    if (!_Mask.ImageData.empty()) {
        const auto selectionMask = BuildSelectionMask(_Mask, Detail::DefaultBinaryMaskPolicy());
        if (!selectionMask.empty()) {
            targetMaskMat.setTo(0, selectionMask);
        }
    } else {
        Info("the Mask is Empty!");
    }

    std::vector<double> targetMeans(_TargetImage.GetBandCounts(), 0.0);
    std::vector<double> targetStdDevs(_TargetImage.GetBandCounts(), 0.0);
    std::vector<double> referenceMeans(_ReferenceImage.GetBandCounts(), 0.0);
    std::vector<double> referenceStdDevs(_ReferenceImage.GetBandCounts(), 0.0);

    cv::meanStdDev(_TargetImage.ImageData, targetMeans, targetStdDevs, targetMaskMat);
    cv::meanStdDev(_ReferenceImage.ImageData, referenceMeans, referenceStdDevs, referenceMaskMat);

    std::vector<cv::Mat> targetChannels;
    cv::split(_TargetImage.ImageData, targetChannels);
    for (size_t channelNum = 0; channelNum < targetChannels.size(); ++channelNum) {
        auto &targetChannel = targetChannels[channelNum];
        const auto &referenceStdDev = referenceStdDevs[channelNum];
        const auto &targetStdDev = targetStdDevs[channelNum];
        const auto &referenceMean = referenceMeans[channelNum];
        const auto &targetMean = targetMeans[channelNum];

        const auto alpha = (targetStdDev < 1e-6) ? 1.0 : (referenceStdDev / targetStdDev);
        const auto beta = referenceMean - alpha * targetMean;
        targetChannel.convertTo(targetChannel, -1, alpha, beta);
    }

    cv::merge(targetChannels, AlgorithmResult.ImageData);
    AlgorithmResult.ImageData.setTo(0, targetNoDataMask);
    AlgorithmResult.ImageName = _TargetImage.ImageName;
    AlgorithmResult.NoDataValues = _TargetImage.NoDataValues;
    AlgorithmResult.GeoInfo = _TargetImage.GeoInfo;
}

} // namespace RSPIP::Algorithm::ColorBalanceAlgorithm
