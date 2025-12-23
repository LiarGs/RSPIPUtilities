#pragma once
#include "Util/ImageShow.h"
#include "Basic/Image.h"
#include "Util/SuperDebug.hpp"
#include <opencv2/highgui.hpp>
// #include <opencv2/imgproc.hpp>

namespace RSPIP::Util {

void ShowImage(const Image &image, int band) {
    if (image.ImageData.empty()) {
        Error("Image data is empty.");
        return;
    }

    int channels = image.GetBandCounts();
    cv::Mat displayImg;
    std::string windowName = image.ImageName;

    if (band > 0) {
        if (band > channels) {
            Error("Invalid band number: {} (Image has {} bands)", band, channels);
            throw std::runtime_error("Invalid band number: " + std::to_string(band));
        }

        cv::extractChannel(image.ImageData, displayImg, band - 1);
        windowName += " Band " + std::to_string(band);
    } else {
        if (channels > 4) {
            // 多光谱数据：提取前三个波段作为 BGR 显示
            Warn("Image has {} bands. Showing first 3 bands as BGR.", channels);
            displayImg.create(image.Height(), image.Width(), CV_MAKETYPE(image.ImageData.depth(), 3));
            int fromTo[] = {0, 0, 1, 1, 2, 2};
            cv::mixChannels(&image.ImageData, 1, &displayImg, 1, fromTo, 3);
        } else {
            displayImg = image.ImageData;
        }
    }

    if (!displayImg.empty()) {
        cv::namedWindow(windowName, cv::WINDOW_NORMAL);
        cv::imshow(windowName, displayImg);
        cv::waitKey(0);
        cv::destroyWindow(windowName);
    }
}

} // namespace RSPIP::Util