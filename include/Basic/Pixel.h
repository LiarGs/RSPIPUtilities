#pragma once

namespace RSPIP {

template <typename T>
struct Pixel {
  public:
    Pixel() : Value(0), Row(-1), Column(-1) {}
    Pixel(T value) : Value(value), Row(0), Column(0) {}
    Pixel(T value, int row, int col) : Value(value), Row(row), Column(col) {}

  public:
    T Value;
    int Row, Column;
};

} // namespace RSPIP