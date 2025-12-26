#pragma once
#include "Algorithm/ImageReconstruct/ReconstructAlgorithmBase.h"
#include "Basic/CloudMask.h"

namespace RSPIP::Algorithm::ReconstructAlgorithm {

class Simple : public ReconstructAlgorithmBase {
  public:
    Simple(const Image &reconstructImage, const Image &referImage, const CloudMask &maskImage);
    void Execute() override;

  private:
    const Image &_ReconstructImage;
    const Image &_ReferImage;
    const CloudMask &_Mask;
};
} // namespace RSPIP::Algorithm::ReconstructAlgorithm