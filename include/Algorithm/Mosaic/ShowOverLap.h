#pragma once
#include "Algorithm/Mosaic/MosaicAlgorithmBase.h"

namespace RSPIP::Algorithm::MosaicAlgorithm {

class ShowOverLap : public MosaicAlgorithmBase {
  public:
    explicit ShowOverLap(std::vector<Image> imageDatas);
    void Execute() override;

  private:
    void _PasteImageToMosaicResult(const Image &imageData) override;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm
