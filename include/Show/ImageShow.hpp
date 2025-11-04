#pragma once
#include "ImageData.hpp"

namespace RSPIP {

inline void PrintImageInfo(const std::shared_ptr<ImageData> &image) {
    if (!image) {
        throw std::runtime_error("Image pointer is null.");
    }
    Info("Image Name: {} Dimensions: {} x {}", image->ImageName, image->Width(),
         image->Height());
    Info("Number of Bands: {}", image->GetBandCounts());
    // GeoInfo
    if (!image->Projection.empty()) {
        Info("Projection:\n {}", image->Projection);
    }
    if (image->GeoTransform.size() == 6) {
        Info("GeoTransform:\n [{}, {}, {}, {}, {}, {}]", image->GeoTransform[0],
             image->GeoTransform[1], image->GeoTransform[2],
             image->GeoTransform[3], image->GeoTransform[4],
             image->GeoTransform[5]);
    }
    Info("Latitude/Longitude of Top-Left Pixel (0,0): ({}, {})",
         image->GetLatitude(0, 0), image->GetLongitude(0, 0));
}

inline void ShowImage(const std::shared_ptr<ImageData> &image, size_t band) {
    if (!image) {
        throw std::runtime_error("Image pointer is null.");
    }
    if (band < 1 || band > static_cast<size_t>(image->GetBandCounts())) {
        throw std::runtime_error("Invalid band number: " +
                                 std::to_string(band));
    }

    const auto &displayImg = image->BGRData;

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
    if (!image->BGRData.data) {
        throw std::runtime_error("Image pointer is null.");
    }

    cv::namedWindow(image->ImageName, cv::WINDOW_NORMAL);
    cv::imshow(image->ImageName, image->BGRData);
    PrintImageInfo(image);

    cv::waitKey(0);
}

} // namespace RSPIP