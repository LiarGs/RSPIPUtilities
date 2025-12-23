#pragma once
#include "Algorithm/Mosaic/MosaicAlgorithmBase.h"

namespace RSPIP::Algorithm::MosaicAlgorithm {

class ShowOverLap : public MosaicAlgorithmBase {
  public:
    explicit ShowOverLap(const std::vector<GeoImage> &imageDatas);
    void Execute() override;

  private:
    void _PasteImageToMosaicResult(const GeoImage &imageData) override;
};
} // namespace RSPIP::Algorithm::MosaicAlgorithm