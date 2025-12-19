#pragma once
#include "Basic/GeoImage.hpp"
#include "Interface/IAlgorithm.h"

namespace RSPIP::Algorithm::ColorBalanceAlgorithm {

class ColorBalanceAlgorithmBase : public IAlgorithm {
  public:
    ColorBalanceAlgorithmBase(const Image &targetImage, const Image &referenceImage)
        : _TargetImage(targetImage), _ReferenceImage(referenceImage) {}

    virtual ~ColorBalanceAlgorithmBase() = default;

  protected:
    ColorBalanceAlgorithmBase() = default;
    ColorBalanceAlgorithmBase(const ColorBalanceAlgorithmBase &) = default;
    ColorBalanceAlgorithmBase(ColorBalanceAlgorithmBase &&) = default;

  protected:
    const Image &_TargetImage;
    const Image &_ReferenceImage;
};

} // namespace RSPIP::Algorithm::ColorBalanceAlgorithm
