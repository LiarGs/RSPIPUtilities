#pragma once
#include "Algorithm/Evaluation/EvaluatorAlgorithmBase.h"
#include "Basic/MaskSelectionPolicy.h"

namespace RSPIP::Algorithm {

// 基于边界梯度的评估算法
class BoundaryGradientEvaluator : public EvaluatorAlgorithmBase {
  public:
    BoundaryGradientEvaluator(Image imageData, Image referenceImage, Image maskImage);

    void Execute() override;

    void SetMaskSelectionPolicy(const MaskSelectionPolicy &policy) {
        _MaskSelectionPolicy = policy;
    }

  public:
    double EvaluateResult;

  private:
    Image _ReferenceImage;
    Image _Mask;
    MaskSelectionPolicy _MaskSelectionPolicy = {};
};

} // namespace RSPIP::Algorithm
