#pragma once
#include "Basic/Image.h"
#include "Interface/IAlgorithm.h"

namespace RSPIP::Algorithm::ReconstructAlgorithm {

class ReconstructAlgorithmBase : public IAlgorithm {
  protected:
    ReconstructAlgorithmBase() = default;
    ReconstructAlgorithmBase(const ReconstructAlgorithmBase &) = default;
    ReconstructAlgorithmBase(ReconstructAlgorithmBase &&) = default;

  public:
    Image AlgorithmResult;
};

} // namespace RSPIP::Algorithm::ReconstructAlgorithm