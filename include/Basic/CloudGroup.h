#pragma once
#include "GeoPixel.h"
#include <opencv2/opencv.hpp>

namespace RSPIP {

struct CloudGroup {
  public:
    cv::Rect CloudRect;
    // 行索引的云像素
    std::map<size_t, std::vector<GeoPixel<uchar>>> CloudPixelMap;
};

} // namespace RSPIP