#pragma once
#include "Algorithm/Evaluation/EvaluatorAlgorithmBase.h"

namespace RSPIP::Algorithm {

// 基于均方根误差的评估算法
class RMSEEvaluator : public EvaluatorAlgorithmBase {
  public:
    RMSEEvaluator(const Image &imageData, const Image &referenceImage);
    RMSEEvaluator(const Image &imageData, const Image &referenceImage, const Image &maskImage);

    void Execute() override;

    void SetBoundaryOnly(bool newBoundaryOnly) {
        _BoundaryOnly = newBoundaryOnly;
    }

  public:
    double EvaluateResult;

  private:
    const Image &_ReferenceImage;
    const Image *_Mask = nullptr;
    bool _BoundaryOnly = false;
};

} // namespace RSPIP::Algorithm
