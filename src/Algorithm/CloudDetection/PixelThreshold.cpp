#include "Algorithm/CloudDetection/PixelThreshold.h"
#include "Util/SuperDebug.hpp"
#include <opencv2/imgproc.hpp>

namespace RSPIP::Algorithm::CloudDetectionAlgorithm {

PixelThreshold::PixelThreshold(const Image &imageData)
    : CloudDetectionAlgorithmBase(imageData) {}

PixelThreshold::PixelThreshold(const GeoImage &imageData)
    : CloudDetectionAlgorithmBase(imageData) {}

void PixelThreshold::SetThreshold(unsigned char threshold) {
    _Threshold = threshold;
}

void PixelThreshold::SetThresholdMode(PixelThresholdMode mode) {
    _Mode = mode;
}

void PixelThreshold::Execute() {
    if (_Image.ImageData.empty()) {
        SuperDebug::Error("Input image is empty in PixelThreshold cloud detection.");
        return;
    }

    cv::Mat thresholdSource;
    if (_Image.ImageData.channels() == 1) {
        thresholdSource = _Image.ImageData;
    } else if (_Image.ImageData.channels() == 3 || _Image.ImageData.channels() == 4) {
        if (_Mode == PixelThresholdMode::BlueBandOnly) {
            cv::extractChannel(_Image.ImageData, thresholdSource, 0);
        } else {
            const auto colorConvertCode = _Image.ImageData.channels() == 3 ? cv::COLOR_BGR2GRAY : cv::COLOR_BGRA2GRAY;
            cv::cvtColor(_Image.ImageData, thresholdSource, colorConvertCode);
        }
    } else {
        SuperDebug::Error("Unsupported channel count: {}", _Image.ImageData.channels());
        return;
    }

    cv::Mat cloudMask;
    cv::threshold(thresholdSource, cloudMask, _Threshold, 255, cv::THRESH_BINARY);

    AlgorithmResult = CloudMask(cloudMask);

    // GeoImage 输入时，复制地理参考信息到输出云掩膜。
    if (_GeoImage != nullptr) {
        AlgorithmResult.SetSourceImage(*_GeoImage);
    }
}

} // namespace RSPIP::Algorithm::CloudDetectionAlgorithm
