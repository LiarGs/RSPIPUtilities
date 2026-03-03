#pragma once
#include "Algorithm/CloudDetection/CloudDetectionAlgorithmBase.h"

namespace RSPIP::Algorithm::CloudDetectionAlgorithm {

class PixelThreshold : public CloudDetectionAlgorithmBase {
  public:
    explicit PixelThreshold(const Image &imageData, unsigned char threshold = 240);
    explicit PixelThreshold(const GeoImage &imageData, unsigned char threshold = 240);

    void Execute() override;

    void SetThreshold(unsigned char threshold) {
        _Threshold = threshold;
    }

  private:
    unsigned char _Threshold;
};

} // namespace RSPIP::Algorithm::CloudDetectionAlgorithm
