#pragma once
#include "Pixel.hpp"

namespace RSPIP {

template <typename T>
struct GeoPixel : public Pixel<T> {
  public:
    GeoPixel() : Pixel<T>(), Latitude(0), Longitude(0) {}
    GeoPixel(T value, size_t row, size_t col, double lat, double lon) : Pixel<T>(value, row, col), Latitude(lat), Longitude(lon) {}

  public:
    double Latitude;
    double Longitude;
};
} // namespace RSPIP