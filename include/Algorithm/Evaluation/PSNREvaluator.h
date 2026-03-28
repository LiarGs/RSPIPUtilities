#pragma once
#include "Algorithm/Evaluation/EvaluatorAlgorithmBase.h"

namespace RSPIP::Algorithm {

// 基于峰值信噪比的评估算法
class PSNREvaluator : public EvaluatorAlgorithmBase {
  public:
    PSNREvaluator(const Image &imageData, const Image &referenceImage);
    PSNREvaluator(const Image &imageData, const Image &referenceImage, const Image &maskImage);

    void Execute() override;

  public:
    double EvaluateResult;

  private:
    const Image &_ReferenceImage;
    const Image *_Mask = nullptr;
};

} // namespace RSPIP::Algorithm
