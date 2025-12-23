#pragma once
#include "Interface/IGeoTransformer.h"
#include <algorithm>
#include <type_traits>
#include <vector>

namespace RSPIP::Util {

template <typename T>
void SortImagesByLongitude(std::vector<T> &ImageDatas) {
    // 确保 T 继承自 IGeoTransformer
    static_assert(std::is_base_of_v<IGeoTransformer, T>, "T must derive from IGeoTransformer");

    std::sort(ImageDatas.begin(), ImageDatas.end(),
              [](const T &a, const T &b) {
                  return a.GetLongitude(0, 0) < b.GetLongitude(0, 0);
              });
}

} // namespace RSPIP::Util