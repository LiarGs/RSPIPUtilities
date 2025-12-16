#pragma once
#include <memory>

namespace RSPIP::Algorithm {

struct AlgorithmParamBase {
  public:
    virtual ~AlgorithmParamBase() = default;

  protected:
    AlgorithmParamBase() = default;
    AlgorithmParamBase(const AlgorithmParamBase &) = default;
    AlgorithmParamBase(AlgorithmParamBase &&) = default;
};

class IAlgorithm {
  public:
    virtual void Execute(std::shared_ptr<AlgorithmParamBase> params) = 0;

  protected:
    IAlgorithm() = default;
    IAlgorithm(const IAlgorithm &) = default;
    IAlgorithm(IAlgorithm &&) = default;

  public:
    std::shared_ptr<GeoImage> AlgorithmResult;
};

} // namespace RSPIP::Algorithm
