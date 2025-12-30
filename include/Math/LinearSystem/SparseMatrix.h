#pragma once
#include "LinearMatrixBase.h"
#include <opencv2/core.hpp>
#include <vector>

namespace RSPIP::Math::LinearSystem {

/**
 * @brief 稀疏矩阵构建辅助结构 (三元组)
 */
struct Triplet {
    Triplet(int r, int c, double v) : Row(r), Col(c), Value(v) {}

    int Row, Col;
    double Value;
};

class SparseMatrix : public LinearMatrixBase {
  public:
    SparseMatrix();
    SparseMatrix(int rows, int cols);
    SparseMatrix(int rows, int cols, const std::vector<Triplet> &triplets);

    void AddTriplet(int row, int col, double value);

    /**
     * @brief 从三元组列表构建 CSR 矩阵
     * @param triplets 三元组列表 (无序)
     * @param rows 矩阵行数
     * @param cols 矩阵列数
     */
    void SetFromTriplets();
    void SetFromTriplets(const std::vector<Triplet> &triplets);

    cv::Mat operator*(const cv::Mat &other) const;

    int Rows() const { return _Rows; }
    int Cols() const { return _Cols; }
    const std::vector<Triplet> &Triplets() const { return _Triplets; }

  private:
    int _Rows;
    int _Cols;
    std::vector<Triplet> _Triplets;

    // CSR 存储结构
    std::vector<double> _Values;
    std::vector<int> _ColIndices;
    std::vector<int> _RowPtrs;
};

} // namespace RSPIP::Math::LinearSystem