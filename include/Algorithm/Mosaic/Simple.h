#pragma once
#include "Algorithm/Mosaic/MosaicAlgorithmBase.h"

namespace RSPIP::Algorithm::MosaicAlgorithm {

class Simple : public MosaicAlgorithmBase {
  public:
    explicit Simple(std::vector<Image> imageDatas);
    void Execute() override;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm
