#pragma once
#include "Algorithm/CloudDetection/CloudDetectionAlgorithmBase.h"

namespace RSPIP::Algorithm::CloudDetectionAlgorithm {

class PixelThreshold : public CloudDetectionAlgorithmBase {
  public:
    explicit PixelThreshold(const Image &imageData);
    explicit PixelThreshold(const GeoImage &imageData);

    void Execute() override;

    void SetThreshold(unsigned char threshold) {
        _Threshold = threshold;
    }

  private:
    unsigned char _Threshold = 240;
};

} // namespace RSPIP::Algorithm::CloudDetectionAlgorithm
