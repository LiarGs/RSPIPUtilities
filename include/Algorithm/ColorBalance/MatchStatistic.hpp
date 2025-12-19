#pragma once
#include "Basic/CloudMask.hpp"
#include "ColorBalanceAlgorithmBase.hpp"

namespace RSPIP::Algorithm::ColorBalanceAlgorithm {

class MatchStatistics : public ColorBalanceAlgorithmBase {
  public:
    MatchStatistics(const Image &targetImage, const Image &referenceImage, const Image &mask = MaskImage())
        : ColorBalanceAlgorithmBase(targetImage, referenceImage), _Mask(mask) {}

    void Execute() override {
        _ColorTransfer();
    }

  private:
    void _ColorTransfer() {
        Info("Executing MatchStatistics ColorBalance...");

        // Exclude extreme pixels (too dark or too bright) from statistics
        // Mask out pixels with pixel < 3 (black) or > 210 (bright)
        cv::Mat maskMat, greyMat;
        cv::cvtColor(_TargetImage.ImageData, greyMat, cv::COLOR_BGR2GRAY);
        cv::inRange(greyMat, 3, 210, maskMat);

        if (!_Mask.ImageData.empty()) {
            maskMat.setTo(0, _Mask.ImageData);
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

  public:
    Image AlgorithmResult;

  private:
    const Image &_Mask;
};

} // namespace RSPIP::Algorithm::ColorBalanceAlgorithm