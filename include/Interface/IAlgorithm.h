#pragma once

namespace RSPIP::Algorithm {

class IAlgorithm {
  public:
    virtual void Execute() = 0;
    virtual ~IAlgorithm() = default;

  protected:
    IAlgorithm() = default;
    IAlgorithm(const IAlgorithm &) = default;
    IAlgorithm(IAlgorithm &&) = default;
};

} // namespace RSPIP::Algorithm
