#pragma once
#include "Algorithm/Evaluation/EvaluatorAlgorithmBase.h"
#include "Basic/MaskSelectionPolicy.h"

#include <optional>

namespace RSPIP::Algorithm {

// 基于结构相似性指数的评估算法
class SSIMEvaluator : public EvaluatorAlgorithmBase {
  public:
    SSIMEvaluator(Image imageData, Image referenceImage);
    SSIMEvaluator(Image imageData, Image referenceImage, Image maskImage);

    void Execute() override;

    void SetK1(double newK1) {
        _K1 = newK1;
    }

    void SetK2(double newK2) {
        _K2 = newK2;
    }

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
    double _K1 = 0.01;
    double _K2 = 0.03;
    MaskSelectionPolicy _MaskSelectionPolicy = {};
};

} // namespace RSPIP::Algorithm
