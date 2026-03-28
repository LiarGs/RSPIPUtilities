#pragma once
#include "Algorithm/Evaluation/EvaluatorAlgorithmBase.h"

namespace RSPIP::Algorithm {

// 基于结构相似性指数的评估算法
class SSIMEvaluator : public EvaluatorAlgorithmBase {
  public:
    SSIMEvaluator(const Image &imageData, const Image &referenceImage);
    SSIMEvaluator(const Image &imageData, const Image &referenceImage, const Image &maskImage);

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

  public:
    double EvaluateResult;

  private:
    const Image &_ReferenceImage;
    const Image *_Mask = nullptr;
    bool _BoundaryOnly = false;
    double _K1 = 0.01;
    double _K2 = 0.03;
};

} // namespace RSPIP::Algorithm
