#pragma once
#include "Basic/Image.h"
#include <algorithm>
#include <limits>
#include <vector>

namespace RSPIP::Util {

inline void SortImagesByLongitude(std::vector<Image> &imageDatas) {
    std::sort(imageDatas.begin(), imageDatas.end(),
              [](const Image &left, const Image &right) {
                  const auto leftLongitude = left.GeoInfo.has_value() ? left.GeoInfo->GetLongitude(0, 0) : std::numeric_limits<double>::max();
                  const auto rightLongitude = right.GeoInfo.has_value() ? right.GeoInfo->GetLongitude(0, 0) : std::numeric_limits<double>::max();
                  return leftLongitude < rightLongitude;
              });
}

} // namespace RSPIP::Util
