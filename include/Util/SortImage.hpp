#pragma once
#include "Basic/GeoImage.hpp"
namespace RSPIP::Util {

void SortImagesByLongitude(std::vector<std::shared_ptr<GeoImage>> &ImageDatas) {
    std::sort(ImageDatas.begin(), ImageDatas.end(),
              [](const std::shared_ptr<GeoImage> &a,
                 const std::shared_ptr<GeoImage> &b) {
                  return a->GetLongitude(0, 0) < b->GetLongitude(0, 0);
              });
}
} // namespace RSPIP::Util
