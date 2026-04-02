#pragma once
#include "Algorithm/Reconstruct/ReconstructAlgorithmBase.h"

namespace RSPIP::Algorithm::ReconstructAlgorithm {

class ColorBalanceReconstruct : public ReconstructAlgorithmBase {
  public:
    ColorBalanceReconstruct(Image reconstructImage, Image referImage, Image maskImage);
    void Execute() override;

  private:
    Image _ReconstructImage;
    Image _ReferImage;
    Image _Mask;
};

} // namespace RSPIP::Algorithm::ReconstructAlgorithm
