#pragma once
#include "Interface/IGeoTransformer.hpp"

namespace RSPIP::Util {

template <typename T>
void SortImagesByLongitude(std::vector<T> &ImageDatas) {
    static_assert(std::is_base_of_v<IGeoTransformer, T>);
    std::sort(ImageDatas.begin(), ImageDatas.end(),
              [](const T &a, const T &b) {
                  return a.GetLongitude(0, 0) < b.GetLongitude(0, 0);
              });
}
} // namespace RSPIP::Util
