#pragma once
#include "Pixel.h"

namespace RSPIP {

template <typename T>
struct GeoPixel : public Pixel<T> {
  public:
    GeoPixel() : Pixel<T>(), Latitude(0), Longitude(0) {}
    GeoPixel(T value, int row, int col) : Pixel<T>(value, row, col), Latitude(0), Longitude(0) {}
    GeoPixel(T value, int row, int col, double lat, double lon) : Pixel<T>(value, row, col), Latitude(lat), Longitude(lon) {}

  public:
    double Latitude;
    double Longitude;
};
} // namespace RSPIP