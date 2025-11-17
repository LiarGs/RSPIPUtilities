#pragma once
#include "Interface/IGeoTransformer.hpp"

namespace RSPIP::Util {

template <typename T>
void SortImagesByLongitude(std::vector<std::shared_ptr<T>> &ImageDatas) {
    static_assert(std::is_base_of_v<IGeoTransformer, T>);
    std::sort(ImageDatas.begin(), ImageDatas.end(),
              [](const std::shared_ptr<T> &a, const std::shared_ptr<T> &b) {
                  return a->GetLongitude(0, 0) < b->GetLongitude(0, 0);
              });
}
} // namespace RSPIP::Util
