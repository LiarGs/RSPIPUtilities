#pragma once
#include "Image.hpp"
#include <opencv2/opencv.hpp>

namespace RSPIP {

template <typename T>
struct Pixel {
  public:
    Pixel() : Value(0), Row(0), Column(0) {}
    Pixel(T value) : Value(value), Row(0), Column(0) {}
    Pixel(T value, size_t row, size_t col) : Value(value), Row(row), Column(col) {}

  public:
    T Value;
    size_t Row, Column;
};

} // namespace RSPIP