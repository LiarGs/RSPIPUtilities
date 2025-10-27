#pragma once
#include "ImageData.hpp"

namespace RSPIP {

inline void ShowImage(const std::shared_ptr<ImageData> &image, size_t band) {
    if (!image) {
        throw std::runtime_error("Image pointer is null.");
    }
    if (band < 1 || band > static_cast<size_t>(image->GetBandCounts())) {
        throw std::runtime_error("Invalid band number: " +
                                 std::to_string(band));
    }

    auto displayImg = image->Data[band - 1];

    if (displayImg.empty()) {
        throw std::runtime_error("Image band " + std::to_string(band) +
                                 " is empty.");
    }

    auto windowName = image->ImageName + " Band " + std::to_string(band);
    cv::namedWindow(windowName, cv::WINDOW_NORMAL);
    cv::imshow(windowName, displayImg);

    cv::waitKey(0);
}

inline void ShowImage(const std::shared_ptr<ImageData> &image) {
    if (!image) {
        throw std::runtime_error("Image pointer is null.");
    }
    if (image->Data.empty()) {
        throw std::runtime_error("No image data to show.");
    }

    cv::namedWindow(image->ImageName, cv::WINDOW_NORMAL);
    cv::imshow(image->ImageName, image->GetMergedData());

    cv::waitKey(0);
}

} // namespace RSPIP