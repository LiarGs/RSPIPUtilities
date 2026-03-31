#include "Algorithm/ColorBalance/MatchStatistic.h"
#include "Basic/GeoImage.h"
#include "Basic/Image.h"
#include "Util/SuperDebug.hpp"
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace RSPIP::Algorithm::ColorBalanceAlgorithm {

MatchStatistics::MatchStatistics(const Image &targetImage, const Image &referenceImage, const Image &mask)
    : ColorBalanceAlgorithmBase(targetImage, referenceImage), _Mask(mask) {}

void MatchStatistics::Execute() {
    // Info("Executing MatchStatistics ColorBalance...");

    if (_TargetImage.ImageData.empty()) {
        Error("Target Image is Empty!");
        return;
    }
    if (_ReferenceImage.ImageData.empty()) {
        Error("Reference Image is Empty!");
        return;
    }

    // Exclude extreme pixels (too dark or too bright) from statistics
    // Mask out pixels with pixel < 3 (black) or > 210 (bright)
    cv::Mat targetMaskMat, targetGreyMat;
    cv::cvtColor(_TargetImage.ImageData, targetGreyMat, cv::COLOR_BGR2GRAY);
    cv::inRange(targetGreyMat, 3, 210, targetMaskMat);
    cv::Mat targetNoDataMask;
    cv::inRange(_TargetImage.ImageData, cv::Scalar::all(0), cv::Scalar::all(0), targetNoDataMask);

    cv::Mat referenceMaskMat, referenceGreyMat;
    cv::cvtColor(_ReferenceImage.ImageData, referenceGreyMat, cv::COLOR_BGR2GRAY);
    cv::inRange(referenceGreyMat, 3, 210, referenceMaskMat);

    if (!_Mask.ImageData.empty()) {
        targetMaskMat.setTo(0, _Mask.ImageData);
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

        auto alpha = (targetStdDev < 1e-6) ? 1.0 : (referenceStdDev / targetStdDev);
        auto beta = referenceMean - alpha * targetMean;
        targetChannel.convertTo(targetChannel, -1, alpha, beta);
    }

    cv::merge(targetChannels, AlgorithmResult.ImageData);
    AlgorithmResult.ImageData.setTo(0, targetNoDataMask);
    AlgorithmResult.ImageName = _TargetImage.ImageName;

    if (const auto *targetGeoImage = dynamic_cast<const GeoImage *>(&_TargetImage)) {
        AlgorithmResult.GeoTransform = targetGeoImage->GeoTransform;
        AlgorithmResult.Projection = targetGeoImage->Projection;
        AlgorithmResult.ImageBounds = targetGeoImage->ImageBounds;
        AlgorithmResult.NonData = targetGeoImage->NonData;
    }

    // Info("ColorBalance Completed.");
}

} // namespace RSPIP::Algorithm::ColorBalanceAlgorithm
