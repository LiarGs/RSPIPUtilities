#pragma once
#include "Basic/Image.hpp"
#include "Interface/IAlgorithm.h"

namespace RSPIP::Algorithm {

class EvaluatorAlgorithmBase : public IAlgorithm {
  public:
    EvaluatorAlgorithmBase(const Image &imageData) : _ImageData(imageData) {}
    virtual ~EvaluatorAlgorithmBase() = default;

  protected:
    EvaluatorAlgorithmBase() = default;
    EvaluatorAlgorithmBase(const EvaluatorAlgorithmBase &) = default;
    EvaluatorAlgorithmBase(EvaluatorAlgorithmBase &&) = default;

  protected:
    const Image &_ImageData;
};

} // namespace RSPIP::Algorithm
