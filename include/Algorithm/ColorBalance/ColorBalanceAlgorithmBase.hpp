#pragma once
#include "Basic/GeoImage.hpp"
#include "Interface/IAlgorithm.h"

namespace RSPIP::Algorithm::ColorBalanceAlgorithm {

struct BasicColorBalanceParam : AlgorithmParamBase {
  public:
    BasicColorBalanceParam(std::shared_ptr<GeoImage> targetImage, std::shared_ptr<GeoImage> referenceImage)
        : TargetImage(targetImage), ReferenceImage(referenceImage) {}

  public:
    std::shared_ptr<GeoImage> TargetImage;
    std::shared_ptr<GeoImage> ReferenceImage;
};

class ColorBalanceAlgorithmBase : public IAlgorithm {
  public:
    virtual ~ColorBalanceAlgorithmBase() = default;

  protected:
    ColorBalanceAlgorithmBase() = default;
    ColorBalanceAlgorithmBase(const ColorBalanceAlgorithmBase &) = default;
    ColorBalanceAlgorithmBase(ColorBalanceAlgorithmBase &&) = default;
};

} // namespace RSPIP::Algorithm::ColorBalanceAlgorithm
