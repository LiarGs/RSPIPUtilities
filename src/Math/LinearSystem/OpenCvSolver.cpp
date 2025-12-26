#include "Math/LinearSystem/OpenCvSolver.h"

namespace RSPIP::Math::LinearSystem {

OpenCvSolver::OpenCvSolver(const cv::Mat &a, const cv::Mat &b, cv::Mat &x) : _A(a), _B(b), _X(x) {}

bool OpenCvSolver::Solve() {
    switch (Config.Method) {
    case SolverMethod::LU:
        return cv::solve(_A, _B, _X, cv::DECOMP_LU);
    case SolverMethod::SVD:
        return cv::solve(_A, _B, _X, cv::DECOMP_SVD);
    case SolverMethod::QR:
        return cv::solve(_A, _B, _X, cv::DECOMP_QR);
    case SolverMethod::Cholesky:
        return cv::solve(_A, _B, _X, cv::DECOMP_CHOLESKY);
    case SolverMethod::Normal:
        return cv::solve(_A, _B, _X, cv::DECOMP_NORMAL);

    default:
        return false;
    }
}

} // namespace RSPIP::Math::LinearSystem