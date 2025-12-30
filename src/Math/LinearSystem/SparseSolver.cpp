#include "Math/LinearSystem/SparseSolver.h"
#include "Util/SuperDebug.hpp"

namespace RSPIP::Math::LinearSystem {

SparseSolver::SparseSolver(const SparseMatrix &a, const cv::Mat &b, cv::Mat &x) : _A(a), _B(b), _X(x) {
    Config.Method = SolverMethod::BiCGSTAB;
    Config.Epsilon = 1e-4;
    Config.MaxIterations = 1000;
}

bool SparseSolver::Solve() {
    switch (Config.Method) {
    case SolverMethod::CG:
        return _ConjugateGradient();
    case SolverMethod::BiCGSTAB:
        return SolveBiCGSTAB();
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
    r -= Ax;                 // residual
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
            SuperDebug::Info("CG Converged at iteration {}. Residual: {}", i + 1, std::sqrt(rsnew));
            return true;
        }

        double beta = rsnew / rsold;
        p = r + beta * p;
        rsold = rsnew;
    }

    SuperDebug::Warn("CG did not converge within {} iterations. Residual: {}", Config.MaxIterations, std::sqrt(rsold));
    return false;
}

bool SparseSolver::SolveBiCGSTAB() const {
    if (_A.Rows() != _B.rows || _A.Cols() != _B.rows) {
        SuperDebug::Error("BiCGSTAB dimension mismatch or non-square matrix.");
        return false;
    }

    if (_X.empty() || _X.rows != _A.Cols() || _X.type() != CV_64F) {
        _X = cv::Mat::zeros(_A.Cols(), 1, CV_64F);
    }

    // r0 = b - Ax
    cv::Mat r = _B - _A * _X;
    cv::Mat r_tilda = r.clone(); // arbitrary vector, usually r0

    cv::Mat p = cv::Mat::zeros(_A.Cols(), 1, CV_64F);
    cv::Mat v = cv::Mat::zeros(_A.Cols(), 1, CV_64F);
    cv::Mat s, t;

    double rho_old = 1.0, alpha = 1.0, omega_old = 1.0;
    double rho_new, beta, omega_new;

    for (int i = 0; i < Config.MaxIterations; ++i) {
        rho_new = r_tilda.dot(r);
        if (std::abs(rho_new) < 1e-16) {
            // 如果 rho 极小，可能已经收敛或失败
            if (cv::norm(r) < Config.Epsilon)
                return true;
            return false;
        }

        if (i == 0) {
            p = r.clone();
        } else {
            beta = (rho_new / rho_old) * (alpha / omega_old);
            p = r + beta * (p - omega_old * v);
        }

        v = _A * p;
        double den = r_tilda.dot(v);
        if (std::abs(den) < 1e-16)
            return false; // Breakdown

        alpha = rho_new / den;
        s = r - alpha * v;

        if (cv::norm(s) < Config.Epsilon) {
            _X += alpha * p;
            SuperDebug::Info("BiCGSTAB Converged at half-step {}", i);
            return true;
        }

        t = _A * s;
        double t_dot_t = t.dot(t);
        if (std::abs(t_dot_t) < 1e-16) {
            _X += alpha * p;
            return true;
        }

        omega_new = t.dot(s) / t_dot_t;
        _X += alpha * p + omega_new * s;
        r = s - omega_new * t;

        if (cv::norm(r) < Config.Epsilon) {
            SuperDebug::Info("BiCGSTAB Converged at step {}", i);
            return true;
        }

        if (std::abs(omega_new) < 1e-16)
            return false; // Breakdown

        rho_old = rho_new;
        omega_old = omega_new;
    }

    SuperDebug::Warn("BiCGSTAB did not converge within {} iterations. Residual: {}", Config.MaxIterations, cv::norm(r));
    return false;
}
} // namespace RSPIP::Math::LinearSystem