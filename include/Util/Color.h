#pragma once
#include <opencv2/core/matx.hpp>

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
} // namespace RSPIP
