#pragma once
#include "Basic/GeoImage.hpp"

namespace RSPIP {

inline void ShowImage(const std::shared_ptr<Image> &image, size_t band) {
    if (!image) {
        throw std::runtime_error("Image pointer is null.");
    }
    if (band < 1 || band > static_cast<size_t>(image->GetBandCounts())) {
        throw std::runtime_error("Invalid band number: " +
                                 std::to_string(band));
    }

    const auto &displayImg = image->ImageData;

    if (displayImg.empty()) {
        throw std::runtime_error("Image band " + std::to_string(band) +
                                 " is empty.");
    }

    auto windowName = image->ImageName + " Band " + std::to_string(band);
    cv::namedWindow(windowName, cv::WINDOW_NORMAL);
    cv::imshow(windowName, displayImg);

    cv::waitKey(0);
}

inline void ShowImage(const std::shared_ptr<Image> &image) {
    if (!image->ImageData.data) {
        throw std::runtime_error("Image pointer is null.");
    }

    cv::namedWindow(image->ImageName, cv::WINDOW_NORMAL);
    cv::imshow(image->ImageName, image->ImageData);
    image->PrintImageInfo();

    cv::waitKey(0);
    cv::destroyWindow(image->ImageName);
}

} // namespace RSPIP