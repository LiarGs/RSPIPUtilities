#pragma once
#include "CloudPixel.h"
#include "Util/SuperDebug.hpp"
#include <algorithm>
#include <map>
#include <opencv2/core/types.hpp>

namespace RSPIP {

struct CloudGroup {
  public:
    CloudPixel<unsigned char> GetCloudPixelByRowColumn(int row, int column) const {
        auto rowIt = CloudPixelMap.find(row);
        if (rowIt == CloudPixelMap.end()) {
            SuperDebug::Error("Wrong Row Index: {} x {}", row, column);
            throw std::runtime_error("Wrong Row Index");
        }

        const auto &rowPixels = rowIt->second;
        auto it = std::lower_bound(rowPixels.begin(), rowPixels.end(), column, [](const CloudPixel<unsigned char> &pixel, int col) {
            return pixel.Column < col;
        });

        if (it != rowPixels.end() && it->Column == column) {
            return *it;
        }

        SuperDebug::Error("Pixel not found at: {} x {}", row, column);
        throw std::runtime_error("Pixel not found");
    }

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
    std::map<int, std::vector<CloudPixel<unsigned char>>> CloudPixelMap;
};

} // namespace RSPIP