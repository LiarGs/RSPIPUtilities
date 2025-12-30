#pragma once
#include "CloudPixel.h"
#include "Util/SuperDebug.hpp"
#include <map>
#include <opencv2/core/types.hpp>

namespace RSPIP {

struct CloudGroup {
  public:
    CloudPixel<unsigned char> GetCloudPixelByRowColumn(int row, int column) const {
        if (CloudPixelMap.find(row) == CloudPixelMap.end()) {
            Error("Wrong Row Index");
            throw std::runtime_error("Wrong Row Index");
        }
        auto startColumn = CloudPixelMap.at(row)[0].Column;
        if (column < startColumn || column >= startColumn + CloudPixelMap.at(row).size()) {
            Error("Wrong Column Index");
            throw std::runtime_error("Wrong Column Index");
        }

        return CloudPixelMap.at(row)[column - startColumn];
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