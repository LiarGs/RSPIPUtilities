#pragma once
#include "GeoPixel.h"

namespace RSPIP {

template <typename T>
struct CloudPixel : public GeoPixel<T> {
  public:
    CloudPixel() : GeoPixel<T>(), PixelNumber(-1) {}
    CloudPixel(T value, int row, int col, int pixelNumber) : GeoPixel<T>(value, row, col), PixelNumber(pixelNumber) {}
    CloudPixel(T value, int row, int col, double lat, double lon) : GeoPixel<T>(value, row, col, lat, lon), PixelNumber(-1) {}
    CloudPixel(T value, int row, int col, double lat, double lon, int pixelNumber) : GeoPixel<T>(value, row, col, lat, lon), PixelNumber(pixelNumber) {}

  public:
    // 在CloudGroup中的序号 从0开始
    int PixelNumber;
};
} // namespace RSPIP