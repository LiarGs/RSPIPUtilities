#pragma once
#include "Interface/IAlgorithm.h"

namespace RSPIP {
class Image;
}

namespace RSPIP::Algorithm {

class EvaluatorAlgorithmBase : public IAlgorithm {
  public:
    explicit EvaluatorAlgorithmBase(const Image &imageData) : _Image(imageData) {}
    virtual ~EvaluatorAlgorithmBase() = default;

  protected:
    EvaluatorAlgorithmBase() = default;
    EvaluatorAlgorithmBase(const EvaluatorAlgorithmBase &) = default;
    EvaluatorAlgorithmBase(EvaluatorAlgorithmBase &&) = default;

  protected:
    const Image &_Image;
};

} // namespace RSPIP::Algorithm