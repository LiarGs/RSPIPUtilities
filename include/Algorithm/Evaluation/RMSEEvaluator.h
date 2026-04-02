#pragma once
#include "Algorithm/Evaluation/EvaluatorAlgorithmBase.h"
#include "Basic/MaskSelectionPolicy.h"

#include <optional>

namespace RSPIP::Algorithm {

// 基于均方根误差的评估算法
class RMSEEvaluator : public EvaluatorAlgorithmBase {
  public:
    RMSEEvaluator(Image imageData, Image referenceImage);
    RMSEEvaluator(Image imageData, Image referenceImage, Image maskImage);

    void Execute() override;

    void SetBoundaryOnly(bool newBoundaryOnly) {
        _BoundaryOnly = newBoundaryOnly;
    }

    void SetMaskSelectionPolicy(const MaskSelectionPolicy &policy) {
        _MaskSelectionPolicy = policy;
    }

  public:
    double EvaluateResult;

  private:
    Image _ReferenceImage;
    std::optional<Image> _Mask = std::nullopt;
    bool _BoundaryOnly = false;
    MaskSelectionPolicy _MaskSelectionPolicy = {};
};

} // namespace RSPIP::Algorithm
