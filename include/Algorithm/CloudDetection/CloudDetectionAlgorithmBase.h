#pragma once
#include "Basic/Image.h"
#include "Interface/IAlgorithm.h"
#include <utility>

namespace RSPIP::Algorithm::CloudDetectionAlgorithm {

class CloudDetectionAlgorithmBase : public IAlgorithm {
  public:
    explicit CloudDetectionAlgorithmBase(Image imageData) : _Image(std::move(imageData)) {}
    virtual ~CloudDetectionAlgorithmBase() = default;

  protected:
    CloudDetectionAlgorithmBase() = default;
    CloudDetectionAlgorithmBase(const CloudDetectionAlgorithmBase &) = default;
    CloudDetectionAlgorithmBase(CloudDetectionAlgorithmBase &&) = default;

  public:
    Image AlgorithmResult;

  protected:
    Image _Image;
};

} // namespace RSPIP::Algorithm::CloudDetectionAlgorithm
