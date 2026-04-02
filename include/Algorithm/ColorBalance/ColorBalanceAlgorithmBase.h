#pragma once
#include "Basic/Image.h"
#include "Interface/IAlgorithm.h"
#include <utility>

namespace RSPIP::Algorithm::ColorBalanceAlgorithm {

class ColorBalanceAlgorithmBase : public IAlgorithm {
  public:
    ColorBalanceAlgorithmBase(Image targetImage, Image referenceImage)
        : _TargetImage(std::move(targetImage)), _ReferenceImage(std::move(referenceImage)) {}

    virtual ~ColorBalanceAlgorithmBase() = default;

  protected:
    ColorBalanceAlgorithmBase() = default;
    ColorBalanceAlgorithmBase(const ColorBalanceAlgorithmBase &) = default;
    ColorBalanceAlgorithmBase(ColorBalanceAlgorithmBase &&) = default;

  protected:
    Image _TargetImage;
    Image _ReferenceImage;
};

} // namespace RSPIP::Algorithm::ColorBalanceAlgorithm
