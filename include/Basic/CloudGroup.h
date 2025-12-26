#pragma once
#include "CloudPixel.h"
#include <map>
#include <opencv2/core/types.hpp>

namespace RSPIP {

struct CloudGroup {
  public:
    int GetNumPixels() const {
        int numPixels = 0;
        for (const auto &[_, rowPixels] : CloudPixelMap) {
            numPixels += rowPixels.size();
        }
        return numPixels;
    }

  public:
    cv::Rect BoundingBox;
    // 行索引的云像素
    std::map<size_t, std::vector<CloudPixel<unsigned char>>> CloudPixelMap;
};

} // namespace RSPIP