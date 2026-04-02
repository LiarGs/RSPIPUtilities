#include "Algorithm/Evaluation/RMSEEvaluator.h"
#include "Algorithm/Detail/AlgorithmValidation.h"
#include "Basic/Image.h"
#include "Basic/RegionExtraction.h"
#include "Util/SuperDebug.hpp"
#include <cmath>
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

cv::Mat _PrepareBoundaryMask(const cv::Mat &binaryMask) {
    if (binaryMask.empty()) {
        return {};
    }

    cv::Mat expandedMask;
    cv::boxFilter(binaryMask, expandedMask, -1, cv::Size(3, 3), cv::Point(-1, -1), true, cv::BORDER_DEFAULT);
    cv::Mat boundaryMask = (expandedMask > 0) & (expandedMask < 255);

    if (boundaryMask.rows > 2 && boundaryMask.cols > 2) {
        boundaryMask.row(0).setTo(0);
        boundaryMask.row(boundaryMask.rows - 1).setTo(0);
        boundaryMask.col(0).setTo(0);
        boundaryMask.col(boundaryMask.cols - 1).setTo(0);
    } else {
        return {};
    }

    return boundaryMask;
}

double _CalculateRMSE(const cv::Mat &image, const cv::Mat &referenceImage, const cv::Mat &mask) {
    cv::Mat diff = image - referenceImage;
    cv::Mat squaredDiff = diff.mul(diff);
    const double mse = mask.empty() ? cv::mean(squaredDiff)[0] : cv::mean(squaredDiff, mask)[0];
    return std::sqrt(mse);
}

} // namespace

namespace RSPIP::Algorithm {

RMSEEvaluator::RMSEEvaluator(Image imageData, Image referenceImage)
    : EvaluatorAlgorithmBase(std::move(imageData)),
      EvaluateResult(std::numeric_limits<double>::quiet_NaN()),
      _ReferenceImage(std::move(referenceImage)) {}

RMSEEvaluator::RMSEEvaluator(Image imageData, Image referenceImage, Image maskImage)
    : EvaluatorAlgorithmBase(std::move(imageData)),
      EvaluateResult(std::numeric_limits<double>::quiet_NaN()),
      _ReferenceImage(std::move(referenceImage)),
      _Mask(std::move(maskImage)) {}

void RMSEEvaluator::Execute() {
    Detail::ResetEvaluationResult(EvaluateResult);
    if (!Detail::ValidateNonEmpty(_Image, "RMSEEvaluator", "input image") ||
        !Detail::ValidateNonEmpty(_ReferenceImage, "RMSEEvaluator", "reference image") ||
        !Detail::ValidateSameSize(_Image, _ReferenceImage, "RMSEEvaluator", "input image", "reference image")) {
        return;
    }

    cv::Mat imageGray = _PrepareGrayImage(_Image.ImageData);
    cv::Mat referenceGray = _PrepareGrayImage(_ReferenceImage.ImageData);
    cv::Mat evaluationMask;
    if (_Mask.has_value()) {
        if (_Mask->ImageData.empty()) {
            SuperDebug::Error("RMSEEvaluator received an empty mask image.");
            return;
        }

        if (!Detail::ValidateSameSize(_Image, *_Mask, "RMSEEvaluator", "input image", "mask image")) {
            return;
        }

        evaluationMask = BuildSelectionMask(*_Mask, _MaskSelectionPolicy);
        if (_BoundaryOnly) {
            evaluationMask = _PrepareBoundaryMask(evaluationMask);
        }
    }

    if (imageGray.empty() || referenceGray.empty() || (_Mask.has_value() && evaluationMask.empty())) {
        SuperDebug::Error("RMSEEvaluator failed to prepare grayscale images or mask.");
        return;
    }

    if (!evaluationMask.empty() && cv::countNonZero(evaluationMask) == 0) {
        if (_BoundaryOnly) {
            SuperDebug::Error("RMSEEvaluator boundary mask contains no valid pixels.");
        } else {
            SuperDebug::Error("RMSEEvaluator mask contains no valid pixels.");
        }
        return;
    }

    EvaluateResult = _CalculateRMSE(imageGray, referenceGray, evaluationMask);
    SuperDebug::Info("RMSE: {:.6f}", EvaluateResult);
}

} // namespace RSPIP::Algorithm
