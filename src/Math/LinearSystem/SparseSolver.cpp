#include "Math/LinearSystem/SparseSolver.h"
#include "Util/SuperDebug.hpp"

namespace RSPIP::Math::LinearSystem {

SparseSolver::SparseSolver(const SparseMatrix &a, const cv::Mat &b, cv::Mat &x) : _A(a), _B(b), _X(x) {}

bool SparseSolver::Solve() {
    switch (Config.Method) {
    case SolverMethod::CG:
        return _ConjugateGradient();
    default:
        return false;
    }
    return false;
}

bool SparseSolver::_ConjugateGradient() const {
    if (_A.Rows() != _B.rows || _A.Cols() != _B.rows) { // CG 只处理方阵
        SuperDebug::Error("CGSolver dimension mismatch or non-square matrix.");
        return false;
    }

    // 初始化 _X (如果是空的，设为零向量；如果不是空的，作为初始猜测)
    if (_X.empty() || _X.rows != _A.Cols() || _X.type() != CV_64F) {
        _X = cv::Mat::zeros(_A.Cols(), 1, CV_64F);
    }

    // CG Algorithm
    // r0 = _B - _A * x0
    auto r = _B.clone();
    auto Ax = _A * _X;
    r -= Ax; // residual

    auto p = r.clone();      // search direction
    auto Ap = _A * p;        // Ap = _A * p
    double rsold = r.dot(r); // r^T * r

    for (int i = 0; i < Config.MaxIterations; ++i) {
        if (std::sqrt(rsold) < Config.Epsilon) {
            SuperDebug::Info("CG Converged at iteration {}", i);
            return true;
        }

        Ap = _A * p;                                // Ap = _A * p
        double alpha = rsold / (p.dot(Ap) + 1e-16); // step size
        _X += alpha * p;
        r -= alpha * Ap;
        double rsnew = r.dot(r);

        // 额外的收敛检查
        if (std::sqrt(rsnew) < Config.Epsilon) {
            SuperDebug::Info("CG Converged at iteration {}", i + 1);
            return true;
        }

        double beta = rsnew / rsold;
        p = r + beta * p;
        rsold = rsnew;
    }

    SuperDebug::Warn("CG did not converge within {} iterations. Residual: {}", Config.MaxIterations, std::sqrt(rsold));
    return false;
}

} // namespace RSPIP::Math::LinearSystem