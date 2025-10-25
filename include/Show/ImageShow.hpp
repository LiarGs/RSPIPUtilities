#pragma once
#include "ImageData.hpp"

namespace RSPIP {

inline void ShowImage(const std::shared_ptr<ImageData> &image) {
    if (!image) {
        throw std::runtime_error("Image pointer is null.");
    }
    if (image->Data.empty()) {
        throw std::runtime_error("No image data to show.");
    }

    for (size_t i = 1; i <= image->GetBandCounts(); ++i) {
        auto displayImg = image->Data[i - 1];

        if (displayImg.empty()) {
            throw std::runtime_error("Image band " + std::to_string(i) +
                                     " is empty.");
        }

        auto windowName = image->ImageName + " Band " + std::to_string(i);
        cv::namedWindow(windowName, cv::WINDOW_NORMAL);
        cv::imshow(windowName, displayImg);

        cv::waitKey(0);
    }

    cv::namedWindow(image->ImageName, cv::WINDOW_NORMAL);
    cv::imshow(image->ImageName, image->GetMergedData());

    cv::waitKey(0);
}

} // namespace RSPIP