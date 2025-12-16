#pragma once
#include "Basic/CloudMask.hpp"
#include "ColorBalanceAlgorithmBase.hpp"

namespace RSPIP::Algorithm::ColorBalanceAlgorithm {

struct MatchStatisticsParams : BasicColorBalanceParam {
  public:
    MatchStatisticsParams(std::shared_ptr<GeoImage> targetImage, std::shared_ptr<GeoImage> referenceImage, std::shared_ptr<CloudMask> mask = nullptr)
        : BasicColorBalanceParam(targetImage, referenceImage), Mask(mask) {}

  public:
    std::shared_ptr<CloudMask> Mask;
};

class MatchStatistics : public ColorBalanceAlgorithmBase {
  public:
    void Execute(std::shared_ptr<AlgorithmParamBase> params) override {
        if (auto matchStatisticsParams = std::dynamic_pointer_cast<MatchStatisticsParams>(params)) {
            AlgorithmResult = matchStatisticsParams->TargetImage;
            _ColorTransfer(matchStatisticsParams->TargetImage, matchStatisticsParams->ReferenceImage, matchStatisticsParams->Mask);
        }
    }

  private:
    void _ColorTransfer(std::shared_ptr<const GeoImage> targetImage, const std::shared_ptr<const GeoImage> referenceImage, const std::shared_ptr<const CloudMask> maskImage) {
        Info("Executing MatchStatistics ColorBalance...");

        // exclude extreme pixels (too dark or too bright) from statistics
        // Mask out pixels with pixel < 3 (black) or > 210 (bright)
        cv::Mat maskMat, greyMat;
        cv::cvtColor(targetImage->ImageData, greyMat, cv::COLOR_BGR2GRAY);
        cv::inRange(greyMat, 3, 210, maskMat);

        if (maskImage != nullptr) {
            maskMat.setTo(0, maskImage->ImageData);
        }

        std::vector<double> targetMeans(targetImage->GetBandCounts(), 0.0);
        std::vector<double> targetStdDevs(targetImage->GetBandCounts(), 0.0);
        std::vector<double> referenceMeans(referenceImage->GetBandCounts(), 0.0);
        std::vector<double> referenceStdDevs(referenceImage->GetBandCounts(), 0.0);

        // meanStdDev 默认非0像素为有效区域，直接使用 maskMat 即可
        cv::meanStdDev(targetImage->ImageData, targetMeans, targetStdDevs, maskMat);
        cv::meanStdDev(referenceImage->ImageData, referenceMeans, referenceStdDevs, maskMat);

        std::vector<cv::Mat> targetChannels;
        cv::split(targetImage->ImageData, targetChannels);
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

        cv::merge(targetChannels, AlgorithmResult->ImageData);

        Info("ColorBalance Completed.");
    }
};

} // namespace RSPIP::Algorithm::ColorBalanceAlgorithm