#pragma once
#include "Interface/IAlgorithm.h"

namespace RSPIP::Algorithm::FilterAlgorithm {

struct BasicFilterParam : AlgorithmParamBase {
  public:
    BasicFilterParam() = default;
};
class FilterAlgorithmBase : public IAlgorithm {
  protected:
    FilterAlgorithmBase() = default;
    FilterAlgorithmBase(const FilterAlgorithmBase &) = default;
    FilterAlgorithmBase(FilterAlgorithmBase &&) = default;
};

} // namespace RSPIP::Algorithm::FilterAlgorithm