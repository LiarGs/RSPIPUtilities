#include "Algorithm/Evaluation/SSIMEvaluator.h"
#include "Basic/Image.h"
#include "Util/SuperDebug.hpp"
#include <algorithm>
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

double _CalculateSSIM(const cv::Mat &image, const cv::Mat &referenceImage, const cv::Mat &mask, double k1, double k2) {
    double imageMin = 0.0;
    double imageMax = 0.0;
    double referenceMin = 0.0;
    double referenceMax = 0.0;
    if (mask.empty()) {
        cv::minMaxLoc(image, &imageMin, &imageMax);
        cv::minMaxLoc(referenceImage, &referenceMin, &referenceMax);
    } else {
        cv::minMaxLoc(image, &imageMin, &imageMax, nullptr, nullptr, mask);
        cv::minMaxLoc(referenceImage, &referenceMin, &referenceMax, nullptr, nullptr, mask);
    }

    double dynamicRange = std::max(imageMax, referenceMax) - std::min(imageMin, referenceMin);
    if (dynamicRange <= std::numeric_limits<double>::epsilon()) {
        dynamicRange = 1.0;
    }

    const double C1 = (k1 * dynamicRange) * (k1 * dynamicRange);
    const double C2 = (k2 * dynamicRange) * (k2 * dynamicRange);

    cv::Mat mu1;
    cv::Mat mu2;
    cv::GaussianBlur(image, mu1, cv::Size(11, 11), 1.5);
    cv::GaussianBlur(referenceImage, mu2, cv::Size(11, 11), 1.5);

    cv::Mat mu1Squared = mu1.mul(mu1);
    cv::Mat mu2Squared = mu2.mul(mu2);
    cv::Mat mu1Mu2 = mu1.mul(mu2);

    cv::Mat sigma1Squared;
    cv::Mat sigma2Squared;
    cv::Mat sigma12;
    cv::GaussianBlur(image.mul(image), sigma1Squared, cv::Size(11, 11), 1.5);
    sigma1Squared -= mu1Squared;
    cv::GaussianBlur(referenceImage.mul(referenceImage), sigma2Squared, cv::Size(11, 11), 1.5);
    sigma2Squared -= mu2Squared;
    cv::GaussianBlur(image.mul(referenceImage), sigma12, cv::Size(11, 11), 1.5);
    sigma12 -= mu1Mu2;

    cv::Mat numerator = (2.0 * mu1Mu2 + C1).mul(2.0 * sigma12 + C2);
    cv::Mat denominator = (mu1Squared + mu2Squared + C1).mul(sigma1Squared + sigma2Squared + C2);

    cv::Mat ssimMap;
    cv::divide(numerator, denominator, ssimMap);

    if (mask.empty()) {
        return cv::mean(ssimMap)[0];
    }

    return cv::mean(ssimMap, mask)[0];
}

} // namespace

namespace RSPIP::Algorithm {

SSIMEvaluator::SSIMEvaluator(const Image &imageData, const Image &referenceImage)
    : EvaluatorAlgorithmBase(imageData),
      EvaluateResult(0.0),
      _ReferenceImage(referenceImage) {}

SSIMEvaluator::SSIMEvaluator(const Image &imageData, const Image &referenceImage, const Image &maskImage)
    : EvaluatorAlgorithmBase(imageData),
      EvaluateResult(0.0),
      _ReferenceImage(referenceImage),
      _Mask(&maskImage) {}

void SSIMEvaluator::Execute() {
    if (_Image.ImageData.empty() || _ReferenceImage.ImageData.empty()) {
        SuperDebug::Error("SSIMEvaluator requires two non-empty input images.");
        return;
    }

    if (_Image.Height() != _ReferenceImage.Height() || _Image.Width() != _ReferenceImage.Width()) {
        SuperDebug::Error("SSIMEvaluator requires images with the same size. Input: {}x{}, Reference: {}x{}",
                          _Image.Width(),
                          _Image.Height(),
                          _ReferenceImage.Width(),
                          _ReferenceImage.Height());
        return;
    }

    if (_K1 <= 0.0 || _K2 <= 0.0) {
        SuperDebug::Error("SSIMEvaluator requires positive stability constants. Current K1: {}, K2: {}",
                          _K1,
                          _K2);
        return;
    }

    cv::Mat imageGray = _PrepareGrayImage(_Image.ImageData);
    cv::Mat referenceGray = _PrepareGrayImage(_ReferenceImage.ImageData);
    cv::Mat evaluationMask;
    if (_Mask != nullptr) {
        if (_Mask->ImageData.empty()) {
            SuperDebug::Error("SSIMEvaluator received an empty mask image.");
            return;
        }

        if (_Mask->Height() != _Image.Height() || _Mask->Width() != _Image.Width()) {
            SuperDebug::Error("SSIMEvaluator requires mask and image sizes to match. Image: {}x{}, Mask: {}x{}",
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
        SuperDebug::Error("SSIMEvaluator failed to prepare grayscale images or mask.");
        return;
    }

    if (!evaluationMask.empty() && cv::countNonZero(evaluationMask) == 0) {
        if (_BoundaryOnly) {
            SuperDebug::Error("SSIMEvaluator boundary mask contains no valid pixels.");
        } else {
            SuperDebug::Error("SSIMEvaluator mask contains no valid pixels.");
        }
        return;
    }

    EvaluateResult = _CalculateSSIM(imageGray, referenceGray, evaluationMask, _K1, _K2);
    SuperDebug::Info("SSIM: {:.6f}", EvaluateResult);
}

} // namespace RSPIP::Algorithm
