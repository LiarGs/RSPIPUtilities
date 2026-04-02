#pragma once
#include "Basic/MaskSelectionPolicy.h"

namespace RSPIP::Algorithm::Detail {

inline MaskSelectionPolicy DefaultBinaryMaskPolicy() {
    MaskSelectionPolicy policy = {};
    policy.Band = 1;
    policy.Mode = MaskSelectionMode::NonZeroSelected;
    policy.SelectedValues.clear();
    return policy;
}

} // namespace RSPIP::Algorithm::Detail
