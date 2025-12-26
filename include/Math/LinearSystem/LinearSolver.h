#pragma once
#include "LinearMatrixBase.h"
#include <memory>
#include <opencv2/core.hpp>

namespace RSPIP::Math::LinearSystem {

/**
 * @brief 求解方法枚举
 */
enum class SolverMethod {
    LU,       // 高斯消元法 (LU分解)，适用于非奇异方阵
    SVD,      // 奇异值分解，适用于奇异矩阵或长方形矩阵 (最小二乘解)
    QR,       // QR分解，适用于最小二乘问题
    Cholesky, // Cholesky分解，仅适用于对称正定矩阵
    CG,       // Conjugate Gradient法，适用于大型正定稀疏矩阵
    Normal    // 正规方程法 (Normal Equations)
};

/**
 * @brief 求解器配置参数
 * 用于传递容差、迭代次数等参数 (主要预留给迭代法或特定算法)
 */
struct SolverConfig {
    SolverMethod Method = SolverMethod::LU;
    double Epsilon = 1e-6;    // 容差 (预留)
    int MaxIterations = 1000; // 最大迭代次数 (预留)
};

/**
 * @brief 线性求解器抽象基类 (Strategy Interface)
 */
class ILinearSolver {
  public:
    virtual ~ILinearSolver() = default;

    /**
     * @brief 求解线性方程组 Ax = B
     * @param A 系数矩阵 (左侧矩阵)
     * @param B 常数矩阵 (右侧矩阵)
     * @param X 输出解矩阵
     * @return true 如果求解成功, false 如果求解失败 (如矩阵奇异)
     */
    virtual bool Solve() = 0;

  protected:
    ILinearSolver() = default;
    ILinearSolver(const ILinearSolver &) = default;
    ILinearSolver(ILinearSolver &&) = default;

  public:
    SolverConfig Config;
};

} // namespace RSPIP::Math::LinearSystem