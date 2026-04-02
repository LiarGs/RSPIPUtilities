#include "Algorithm/Evaluation/BoundaryGradientEvaluator.h"
#include "Algorithm/Detail/AlgorithmValidation.h"
#include "Basic/Image.h"
#include "Basic/RegionExtraction.h"
#include <limits>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

namespace RSPIP::Algorithm {

BoundaryGradientEvaluator::BoundaryGradientEvaluator(Image imageData, Image referenceImage, Image maskImage)
    : EvaluatorAlgorithmBase(std::move(imageData)),
      EvaluateResult(std::numeric_limits<double>::quiet_NaN()),
      _ReferenceImage(std::move(referenceImage)),
      _Mask(std::move(maskImage)) {}

void BoundaryGradientEvaluator::Execute() {
    Detail::ResetEvaluationResult(EvaluateResult);
    if (!Detail::ValidateNonEmpty(_Image, "BoundaryGradientEvaluator", "input image") ||
        !Detail::ValidateNonEmpty(_ReferenceImage, "BoundaryGradientEvaluator", "reference image") ||
        !Detail::ValidateNonEmpty(_Mask, "BoundaryGradientEvaluator", "mask image") ||
        !Detail::ValidateSameSize(_Image, _ReferenceImage, "BoundaryGradientEvaluator", "input image", "reference image") ||
        !Detail::ValidateSameSize(_Image, _Mask, "BoundaryGradientEvaluator", "input image", "mask image")) {
        return;
    }

    auto mask = BuildSelectionMask(_Mask, _MaskSelectionPolicy);
    cv::boxFilter(mask, mask, -1, cv::Size(3, 3), cv::Point(-1, -1), true, cv::BORDER_DEFAULT);
    cv::Mat boundaryMask = (mask > 0) & (mask < 255);

    if (boundaryMask.rows > 2 && boundaryMask.cols > 2) {
        boundaryMask.row(0).setTo(0);
        boundaryMask.row(boundaryMask.rows - 1).setTo(0);
        boundaryMask.col(0).setTo(0);
        boundaryMask.col(boundaryMask.cols - 1).setTo(0);
    } else {
        return;
    }

    auto prepareImage = [](const cv::Mat &src) -> cv::Mat {
        cv::Mat gray;
        if (src.channels() == 3) {
            cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
        } else if (src.channels() == 4) {
            cv::cvtColor(src, gray, cv::COLOR_BGRA2GRAY);
        } else {
            gray = src;
        }

        cv::Mat normalized;
        gray.convertTo(normalized, CV_64F, 1.0 / 255.0);
        return normalized;
    };

    cv::Mat image = prepareImage(_Image.ImageData);
    cv::Mat referenceImage = prepareImage(_ReferenceImage.ImageData);

    cv::Mat kernel = (cv::Mat_<double>(3, 3) << 1, 1, 1, 1, -8, 1, 1, 1, 1) / 8.0;
    cv::filter2D(image, image, -1, kernel);
    cv::filter2D(referenceImage, referenceImage, -1, kernel);

    EvaluateResult = cv::mean(image - referenceImage, boundaryMask)[0];
}

} // namespace RSPIP::Algorithm
