#pragma once
#include "GeoPixel.hpp"

namespace RSPIP {

struct CloudGroup {
  public:
    cv::Rect CloudRect;
    // 行索引的云像素
    std::map<size_t, std::vector<GeoPixel<uchar>>> CloudPixelMap;
};

} // namespace RSPIP