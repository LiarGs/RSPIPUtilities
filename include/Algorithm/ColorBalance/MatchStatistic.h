#pragma once
#include "Algorithm/ColorBalance/ColorBalanceAlgorithmBase.h"
#include "Basic/Image.h"

namespace RSPIP::Algorithm::ColorBalanceAlgorithm {

class MatchStatistics : public ColorBalanceAlgorithmBase {
  public:
    MatchStatistics(Image targetImage, Image referenceImage, Image mask = Image());

    void Execute() override;

  public:
    Image AlgorithmResult;

  private:
    Image _Mask;
};

} // namespace RSPIP::Algorithm::ColorBalanceAlgorithm
