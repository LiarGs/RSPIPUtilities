#pragma once

namespace RSPIP::Math::LinearSystem {

class LinearMatrixBase {
  public:
    /**
     * @brief 执行矩阵向量乘法 y = A * x
     * @param x 输入向量 (列向量)
     * @param y 输出向量 (列向量)
     */

    virtual ~LinearMatrixBase() = default;

  protected:
    LinearMatrixBase() = default;
};

} // namespace RSPIP::Math::LinearSystem
