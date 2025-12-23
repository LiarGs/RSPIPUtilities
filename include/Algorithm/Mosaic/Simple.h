#pragma once
#include "Algorithm/Mosaic/MosaicAlgorithmBase.h"

namespace RSPIP::Algorithm::MosaicAlgorithm {

class Simple : public MosaicAlgorithmBase {
  public:
    explicit Simple(const std::vector<GeoImage> &imageDatas);
    void Execute() override;
};
} // namespace RSPIP::Algorithm::MosaicAlgorithm