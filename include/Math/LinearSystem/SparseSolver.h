#pragma once
#include "LinearSolver.h"
#include "SparseMatrix.h"

namespace RSPIP::Math::LinearSystem {
class SparseSolver : public ILinearSolver {
  public:
    SparseSolver(const SparseMatrix &a, const cv::Mat &b, cv::Mat &x);

    bool Solve() override;

  private:
    bool _ConjugateGradient() const;

  private:
    const SparseMatrix &_A;
    const cv::Mat &_B;
    cv::Mat &_X;
};

} // namespace RSPIP::Math::LinearSystem