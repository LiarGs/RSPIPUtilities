#pragma once
#include <opencv2/opencv.hpp>

namespace RSPIP {

struct Color { // BGR
    static const cv::Vec3b Red;
    static const cv::Vec3b Green;
    static const cv::Vec3b Blue;
    static const cv::Vec3b White;
    static const cv::Vec3b Black;
    static const cv::Vec3b Yellow;
    static const cv::Vec3b Cyan;
    static const cv::Vec3b Magenta;
};

const cv::Vec3b Color::Red = {0, 0, 255};
const cv::Vec3b Color::Green = {0, 255, 0};
const cv::Vec3b Color::Blue = {255, 0, 0};
const cv::Vec3b Color::White = {255, 255, 255};
const cv::Vec3b Color::Black = {0, 0, 0};
const cv::Vec3b Color::Yellow = {0, 255, 255};
const cv::Vec3b Color::Cyan = {255, 255, 0};
const cv::Vec3b Color::Magenta = {255, 0, 255};

} // namespace RSPIP