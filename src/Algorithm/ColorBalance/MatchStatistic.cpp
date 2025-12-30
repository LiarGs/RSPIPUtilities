#include "Algorithm/ColorBalance/MatchStatistic.h"
#include "Basic/Image.h"
#include "Util/SuperDebug.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace RSPIP::Algorithm::ColorBalanceAlgorithm {

MatchStatistics::MatchStatistics(const Image &targetImage, const Image &referenceImage, const Image &mask)
    : ColorBalanceAlgorithmBase(targetImage, referenceImage), _Mask(mask) {}

void MatchStatistics::Execute() {
    Info("Executing MatchStatistics ColorBalance...");

    if (_TargetImage.ImageData.empty()) {
        Error("Target Image is Empty!");
    }
    if (_ReferenceImage.ImageData.empty()) {
        Error("Reference Image is Empty!");
    }

    // Exclude extreme pixels (too dark or too bright) from statistics
    // Mask out pixels with pixel < 3 (black) or > 210 (bright)
    cv::Mat maskMat, greyMat;
    cv::cvtColor(_TargetImage.ImageData, greyMat, cv::COLOR_BGR2GRAY);
    cv::inRange(greyMat, 3, 210, maskMat);

    if (!_Mask.ImageData.empty()) {
        maskMat.setTo(0, _Mask.ImageData);
    } else {
        Error("the Mask is Empty!");
    }

    std::vector<double> targetMeans(_TargetImage.GetBandCounts(), 0.0);
    std::vector<double> targetStdDevs(_TargetImage.GetBandCounts(), 0.0);
    std::vector<double> referenceMeans(_ReferenceImage.GetBandCounts(), 0.0);
    std::vector<double> referenceStdDevs(_ReferenceImage.GetBandCounts(), 0.0);

    cv::meanStdDev(_TargetImage.ImageData, targetMeans, targetStdDevs, maskMat);
    cv::meanStdDev(_ReferenceImage.ImageData, referenceMeans, referenceStdDevs, maskMat);

    std::vector<cv::Mat> targetChannels;
    cv::split(_TargetImage.ImageData, targetChannels);
    for (size_t channelNum = 0; channelNum < targetChannels.size(); ++channelNum) {
        auto &targetChannel = targetChannels[channelNum];
        const auto &referenceStdDev = referenceStdDevs[channelNum];
        const auto &targetStdDev = targetStdDevs[channelNum];
        const auto &referenceMean = referenceMeans[channelNum];
        const auto &targetMean = targetMeans[channelNum];

        auto alpha = (targetStdDev < 1e-6) ? 1.0 : (referenceStdDev / targetStdDev);
        auto beta = referenceMean - alpha * targetMean;
        targetChannel.convertTo(targetChannel, -1, alpha, beta);
    }

    cv::merge(targetChannels, AlgorithmResult.ImageData);

    Info("ColorBalance Completed.");
}

} // namespace RSPIP::Algorithm::ColorBalanceAlgorithm