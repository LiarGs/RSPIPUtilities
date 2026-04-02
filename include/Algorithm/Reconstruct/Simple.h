#pragma once
#include "Algorithm/Reconstruct/ReconstructAlgorithmBase.h"

namespace RSPIP::Algorithm::ReconstructAlgorithm {

class Simple : public ReconstructAlgorithmBase {
  public:
    Simple(Image reconstructImage, Image referImage, Image maskImage);
    void Execute() override;

  private:
    Image _ReconstructImage;
    Image _ReferImage;
    Image _Mask;
};

} // namespace RSPIP::Algorithm::ReconstructAlgorithm
