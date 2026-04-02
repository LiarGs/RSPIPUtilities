#pragma once
#include <vector>

namespace RSPIP {

enum class MaskSelectionMode {
    NonZeroSelected,
    ValueSetSelected
};

struct MaskSelectionPolicy {
  public:
    int Band = 1;
    MaskSelectionMode Mode = MaskSelectionMode::NonZeroSelected;
    std::vector<int> SelectedValues = {};
};

} // namespace RSPIP
