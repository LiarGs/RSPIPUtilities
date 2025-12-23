#pragma once
#include "Algorithm/Evaluation/EvaluatorAlgorithmBase.h"

namespace RSPIP::Algorithm {

// 基于边界梯度的评估算法
class BoundaryGradientEvaluator : public EvaluatorAlgorithmBase {
  public:
    BoundaryGradientEvaluator(const Image &imageData, const Image &referenceImage, const Image &maskImage);

    void Execute() override;

  public:
    double EvaluateResult;

  private:
    const Image &_ReferenceImage;
    const Image &_Mask;
};

} // namespace RSPIP::Algorithm