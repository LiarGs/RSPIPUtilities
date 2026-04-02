#pragma once
#include "Basic/RegionPixel.h"
#include <map>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <stdexcept>
#include <vector>

namespace RSPIP {

struct RegionGroup {
  public:
    const RegionPixel &GetPixel(int row, int column) const {
        if (row < BoundingBox.y || row >= BoundingBox.y + BoundingBox.height ||
            column < BoundingBox.x || column >= BoundingBox.x + BoundingBox.width ||
            LocalIndexMap.empty()) {
            throw std::runtime_error("Wrong Row Index");
        }

        const int localIndex = LocalIndexMap.at<int>(row - BoundingBox.y, column - BoundingBox.x);
        if (localIndex < 0 || localIndex >= static_cast<int>(Pixels.size())) {
            throw std::runtime_error("Pixel not found");
        }

        return Pixels[static_cast<size_t>(localIndex)];
    }

    int GetNumPixels() const {
        return PixelCount;
    }

  public:
    cv::Rect BoundingBox;
    int PixelCount = 0;
    cv::Mat LocalIndexMap;
    std::vector<RegionPixel> Pixels;
    std::map<int, std::vector<RegionPixel>> PixelsByRow;
};

} // namespace RSPIP
