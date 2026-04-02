#pragma once

namespace RSPIP {

struct RegionPixel {
  public:
    RegionPixel() = default;
    RegionPixel(int row, int column, int localIndex)
        : Row(row), Column(column), LocalIndex(localIndex) {}

  public:
    int Row = -1;
    int Column = -1;
    int LocalIndex = -1;
};

} // namespace RSPIP
