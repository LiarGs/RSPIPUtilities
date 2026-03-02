#pragma once
#include "Interface/IAlgorithm.h"

namespace RSPIP::Algorithm::FilterAlgorithm {

class FilterAlgorithmBase : public IAlgorithm {
  protected:
    FilterAlgorithmBase() = default;
    FilterAlgorithmBase(const FilterAlgorithmBase &) = default;
    FilterAlgorithmBase(FilterAlgorithmBase &&) = default;
};

} // namespace RSPIP::Algorithm::FilterAlgorithm