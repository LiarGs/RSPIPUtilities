#pragma once
#include "Algorithm/ColorBalance/ColorBalanceAlgorithmBase.h"
#include "Basic/Image.h"

namespace RSPIP::Algorithm::ColorBalanceAlgorithm {

class MatchStatistics : public ColorBalanceAlgorithmBase {
  public:
    MatchStatistics(const Image &targetImage, const Image &referenceImage, const Image &mask = Image());

    void Execute() override;

  public:
    Image AlgorithmResult;

  private:
    const Image &_Mask;
};

} // namespace RSPIP::Algorithm::ColorBalanceAlgorithm