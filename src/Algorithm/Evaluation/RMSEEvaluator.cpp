#include "Algorithm/Evaluation/RMSEEvaluator.h"
#include "Basic/Image.h"
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

cv::Mat _PrepareBinaryMask(const cv::Mat &src) {
    if (src.empty()) {
        return {};
    }

    cv::Mat grayMask;
    if (src.channels() == 1) {
        src.convertTo(grayMask, CV_8U);
    } else {
        cv::Mat srcDouble;
        src.convertTo(srcDouble, CV_64F);

        std::vector<cv::Mat> channels;
        cv::split(srcDouble, channels);

        cv::Mat averagedMask = cv::Mat::zeros(src.rows, src.cols, CV_64F);
        for (const auto &channel : channels) {
            averagedMask += channel;
        }
        averagedMask /= static_cast<double>(channels.size());
        averagedMask.convertTo(grayMask, CV_8U);
    }

    cv::Mat binaryMask;
    cv::threshold(grayMask, binaryMask, 0, 255, cv::THRESH_BINARY);
    return binaryMask;
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

RMSEEvaluator::RMSEEvaluator(const Image &imageData, const Image &referenceImage)
    : EvaluatorAlgorithmBase(imageData),
      EvaluateResult(0.0),
      _ReferenceImage(referenceImage) {}

RMSEEvaluator::RMSEEvaluator(const Image &imageData, const Image &referenceImage, const Image &maskImage)
    : EvaluatorAlgorithmBase(imageData),
      EvaluateResult(0.0),
      _ReferenceImage(referenceImage),
      _Mask(&maskImage) {}

void RMSEEvaluator::Execute() {
    if (_Image.ImageData.empty() || _ReferenceImage.ImageData.empty()) {
        SuperDebug::Error("RMSEEvaluator requires two non-empty input images.");
        return;
    }

    if (_Image.Height() != _ReferenceImage.Height() || _Image.Width() != _ReferenceImage.Width()) {
        SuperDebug::Error("RMSEEvaluator requires images with the same size. Input: {}x{}, Reference: {}x{}",
                          _Image.Width(),
                          _Image.Height(),
                          _ReferenceImage.Width(),
                          _ReferenceImage.Height());
        return;
    }

    cv::Mat imageGray = _PrepareGrayImage(_Image.ImageData);
    cv::Mat referenceGray = _PrepareGrayImage(_ReferenceImage.ImageData);
    cv::Mat evaluationMask;
    if (_Mask != nullptr) {
        if (_Mask->ImageData.empty()) {
            SuperDebug::Error("RMSEEvaluator received an empty mask image.");
            return;
        }

        if (_Mask->Height() != _Image.Height() || _Mask->Width() != _Image.Width()) {
            SuperDebug::Error("RMSEEvaluator requires mask and image sizes to match. Image: {}x{}, Mask: {}x{}",
                              _Image.Width(),
                              _Image.Height(),
                              _Mask->Width(),
                              _Mask->Height());
            return;
        }

        evaluationMask = _PrepareBinaryMask(_Mask->ImageData);
        if (_BoundaryOnly) {
            evaluationMask = _PrepareBoundaryMask(evaluationMask);
        }
    }

    if (imageGray.empty() || referenceGray.empty() || (_Mask != nullptr && evaluationMask.empty())) {
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
