#pragma once
#include "Basic/CloudMask.h"
#include "Basic/GeoImage.h"
#include "Basic/Image.h"
#include "Interface/IAlgorithm.h"

namespace RSPIP::Algorithm::CloudDetectionAlgorithm {

class CloudDetectionAlgorithmBase : public IAlgorithm {
  public:
    explicit CloudDetectionAlgorithmBase(const Image &imageData) : _Image(imageData), _GeoImage(nullptr) {}
    explicit CloudDetectionAlgorithmBase(const GeoImage &imageData) : _Image(imageData), _GeoImage(&imageData) {}
    virtual ~CloudDetectionAlgorithmBase() = default;

  protected:
    CloudDetectionAlgorithmBase() = default;
    CloudDetectionAlgorithmBase(const CloudDetectionAlgorithmBase &) = default;
    CloudDetectionAlgorithmBase(CloudDetectionAlgorithmBase &&) = default;

  public:
    CloudMask AlgorithmResult;

  protected:
    const Image &_Image;
    const GeoImage *_GeoImage;
};

} // namespace RSPIP::Algorithm::CloudDetectionAlgorithm
