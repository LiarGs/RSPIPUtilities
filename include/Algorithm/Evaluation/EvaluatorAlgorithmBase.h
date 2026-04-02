#pragma once
#include "Basic/Image.h"
#include "Interface/IAlgorithm.h"
#include <utility>

namespace RSPIP::Algorithm {

class EvaluatorAlgorithmBase : public IAlgorithm {
  public:
    explicit EvaluatorAlgorithmBase(Image imageData) : _Image(std::move(imageData)) {}
    virtual ~EvaluatorAlgorithmBase() = default;

  protected:
    EvaluatorAlgorithmBase() = default;
    EvaluatorAlgorithmBase(const EvaluatorAlgorithmBase &) = default;
    EvaluatorAlgorithmBase(EvaluatorAlgorithmBase &&) = default;

  protected:
    Image _Image;
};

} // namespace RSPIP::Algorithm
