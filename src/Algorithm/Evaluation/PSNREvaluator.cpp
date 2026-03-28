#include "Algorithm/Evaluation/PSNREvaluator.h"
#include "Basic/Image.h"
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

PSNREvaluator::PSNREvaluator(const Image &imageData, const Image &referenceImage)
    : EvaluatorAlgorithmBase(imageData),
      EvaluateResult(0.0),
      _ReferenceImage(referenceImage) {}

PSNREvaluator::PSNREvaluator(const Image &imageData, const Image &referenceImage, const Image &maskImage)
    : EvaluatorAlgorithmBase(imageData),
      EvaluateResult(0.0),
      _ReferenceImage(referenceImage),
      _Mask(&maskImage) {}

void PSNREvaluator::Execute() {
    if (_Image.ImageData.empty() || _ReferenceImage.ImageData.empty()) {
        SuperDebug::Error("PSNREvaluator requires two non-empty input images.");
        return;
    }

    if (_Image.Height() != _ReferenceImage.Height() || _Image.Width() != _ReferenceImage.Width()) {
        SuperDebug::Error("PSNREvaluator requires images with the same size. Input: {}x{}, Reference: {}x{}",
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
            SuperDebug::Error("PSNREvaluator received an empty mask image.");
            return;
        }

        if (_Mask->Height() != _Image.Height() || _Mask->Width() != _Image.Width()) {
            SuperDebug::Error("PSNREvaluator requires mask and image sizes to match. Image: {}x{}, Mask: {}x{}",
                              _Image.Width(),
                              _Image.Height(),
                              _Mask->Width(),
                              _Mask->Height());
            return;
        }

        evaluationMask = _PrepareBinaryMask(_Mask->ImageData);
    }

    if (imageGray.empty() || referenceGray.empty() || (_Mask != nullptr && evaluationMask.empty())) {
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
