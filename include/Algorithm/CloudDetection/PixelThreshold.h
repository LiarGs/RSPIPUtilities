#pragma once
#include "Algorithm/CloudDetection/CloudDetectionAlgorithmBase.h"

namespace RSPIP::Algorithm::CloudDetectionAlgorithm {

enum class PixelThresholdMode {
    Gray,
    BlueBandOnly
};

class PixelThreshold : public CloudDetectionAlgorithmBase {
  public:
    explicit PixelThreshold(const Image &imageData);
    explicit PixelThreshold(const GeoImage &imageData);

    void Execute() override;

    void SetThreshold(unsigned char threshold);
    void SetThresholdMode(PixelThresholdMode mode);

  private:
    unsigned char _Threshold = 240;
    PixelThresholdMode _Mode = PixelThresholdMode::Gray;
};

} // namespace RSPIP::Algorithm::CloudDetectionAlgorithm
