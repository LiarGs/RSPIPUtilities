#pragma once
#include "LinearSolver.h"

namespace RSPIP::Math::LinearSystem {

/**
 * @brief OpenCV 自带求解器
 */
class OpenCvSolver : public ILinearSolver {
  public:
    OpenCvSolver(const cv::Mat &a, const cv::Mat &b, cv::Mat &x);

    bool Solve() override;

  public:
    const cv::Mat &_A, &_B;
    cv::Mat &_X;
};
} // namespace RSPIP::Math::LinearSystem