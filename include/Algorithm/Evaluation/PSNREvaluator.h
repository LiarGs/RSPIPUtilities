#pragma once
#include "Algorithm/Evaluation/EvaluatorAlgorithmBase.h"
#include "Basic/MaskSelectionPolicy.h"
#include <optional>

namespace RSPIP::Algorithm {

// 基于峰值信噪比的评估算法
class PSNREvaluator : public EvaluatorAlgorithmBase {
  public:
    PSNREvaluator(Image imageData, Image referenceImage);
    PSNREvaluator(Image imageData, Image referenceImage, Image maskImage);

    void Execute() override;

    void SetMaskSelectionPolicy(const MaskSelectionPolicy &policy) {
        _MaskSelectionPolicy = policy;
    }

  public:
    double EvaluateResult;

  private:
    Image _ReferenceImage;
    std::optional<Image> _Mask = std::nullopt;
    MaskSelectionPolicy _MaskSelectionPolicy = {};
};

} // namespace RSPIP::Algorithm
