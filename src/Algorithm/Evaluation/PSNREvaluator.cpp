#include "Algorithm/Evaluation/PSNREvaluator.h"
#include "Algorithm/Detail/AlgorithmValidation.h"
#include "Basic/Image.h"
#include "Basic/RegionExtraction.h"
#include "Util/SuperDebug.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

namespace {

cv::Mat _PrepareGrayImage(const cv::Mat &src) {
    if (src.empty()) {
        return {};
    }

    cv::Mat srcDouble;
    src.convertTo(srcDouble, CV_64F);

    if (srcDouble.channels() == 1) {
        return srcDouble;
    }

    std::vector<cv::Mat> channels;
    cv::split(srcDouble, channels);

    cv::Mat gray = cv::Mat::zeros(src.rows, src.cols, CV_64F);
    for (const auto &channel : channels) {
        gray += channel;
    }
    gray /= static_cast<double>(channels.size());

    return gray;
}

double _CalculatePSNR(const cv::Mat &image, const cv::Mat &referenceImage, const cv::Mat &mask) {
    cv::Mat diff = image - referenceImage;
    cv::Mat squaredDiff = diff.mul(diff);
    const double mse = mask.empty() ? cv::mean(squaredDiff)[0] : cv::mean(squaredDiff, mask)[0];

    if (mse <= std::numeric_limits<double>::epsilon()) {
        return std::numeric_limits<double>::infinity();
    }

    double imageMax = 0.0;
    double referenceMax = 0.0;
    if (mask.empty()) {
        cv::minMaxLoc(image, nullptr, &imageMax);
        cv::minMaxLoc(referenceImage, nullptr, &referenceMax);
    } else {
        cv::minMaxLoc(image, nullptr, &imageMax, nullptr, nullptr, mask);
        cv::minMaxLoc(referenceImage, nullptr, &referenceMax, nullptr, nullptr, mask);
    }

    const double peakValue = std::max({imageMax, referenceMax, 1.0});
    return 10.0 * std::log10((peakValue * peakValue) / mse);
}

} // namespace

namespace RSPIP::Algorithm {

PSNREvaluator::PSNREvaluator(Image imageData, Image referenceImage)
    : EvaluatorAlgorithmBase(std::move(imageData)),
      EvaluateResult(std::numeric_limits<double>::quiet_NaN()),
      _ReferenceImage(std::move(referenceImage)) {}

PSNREvaluator::PSNREvaluator(Image imageData, Image referenceImage, Image maskImage)
    : EvaluatorAlgorithmBase(std::move(imageData)),
      EvaluateResult(std::numeric_limits<double>::quiet_NaN()),
      _ReferenceImage(std::move(referenceImage)),
      _Mask(std::move(maskImage)) {}

void PSNREvaluator::Execute() {
    Detail::ResetEvaluationResult(EvaluateResult);
    if (!Detail::ValidateNonEmpty(_Image, "PSNREvaluator", "input image") ||
        !Detail::ValidateNonEmpty(_ReferenceImage, "PSNREvaluator", "reference image") ||
        !Detail::ValidateSameSize(_Image, _ReferenceImage, "PSNREvaluator", "input image", "reference image")) {
        return;
    }

    cv::Mat imageGray = _PrepareGrayImage(_Image.ImageData);
    cv::Mat referenceGray = _PrepareGrayImage(_ReferenceImage.ImageData);
    cv::Mat evaluationMask;
    if (_Mask.has_value()) {
        if (_Mask->ImageData.empty()) {
            SuperDebug::Error("PSNREvaluator received an empty mask image.");
            return;
        }

        if (!Detail::ValidateSameSize(_Image, *_Mask, "PSNREvaluator", "input image", "mask image")) {
            return;
        }

        evaluationMask = BuildSelectionMask(*_Mask, _MaskSelectionPolicy);
    }

    if (imageGray.empty() || referenceGray.empty() || (_Mask.has_value() && evaluationMask.empty())) {
        SuperDebug::Error("PSNREvaluator failed to prepare grayscale images or mask.");
        return;
    }

    if (!evaluationMask.empty() && cv::countNonZero(evaluationMask) == 0) {
        SuperDebug::Error("PSNREvaluator mask contains no valid pixels.");
        return;
    }

    EvaluateResult = _CalculatePSNR(imageGray, referenceGray, evaluationMask);
    if (std::isinf(EvaluateResult)) {
        SuperDebug::Info("PSNR: inf dB");
        return;
    }

    SuperDebug::Info("PSNR: {:.6f} dB", EvaluateResult);
}

} // namespace RSPIP::Algorithm
