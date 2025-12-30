#include "Math/LinearSystem/SparseMatrix.h"
#include "Util/SuperDebug.hpp"
#include <map>

namespace RSPIP::Math::LinearSystem {

SparseMatrix::SparseMatrix() : _Rows(0), _Cols(0) {}

SparseMatrix::SparseMatrix(int rows, int cols) : _Rows(rows), _Cols(cols) {
    _RowPtrs.resize(_Rows + 1, 0);
}

SparseMatrix::SparseMatrix(int rows, int cols, const std::vector<Triplet> &triplets) : _Rows(rows), _Cols(cols) {
    SetFromTriplets(triplets);
}

void SparseMatrix::AddTriplet(int row, int col, double value) {
    _Triplets.emplace_back(row, col, value);
}

void SparseMatrix::SetFromTriplets() {
    if (_Triplets.empty()) {
        Error("Triplets is empty");
        return;
    }
    _Values.clear();
    _ColIndices.clear();
    _RowPtrs.clear();

    std::sort(_Triplets.begin(), _Triplets.end(), [](const Triplet &a, const Triplet &b) {
        if (a.Row != b.Row)
            return a.Row < b.Row;
        return a.Col < b.Col;
    });

    // 构建 CSR
    _RowPtrs.resize(_Rows + 1, 0);
    _Values.reserve(_Triplets.size());
    _ColIndices.reserve(_Triplets.size());

    int currentTripIdx = 0;
    for (int currentRow = 0; currentRow < _Rows; ++currentRow) {
        _RowPtrs[currentRow] = static_cast<int>(_Values.size());

        while (currentTripIdx < _Triplets.size() && _Triplets[currentTripIdx].Row == currentRow) {
            const auto &trip = _Triplets[currentTripIdx++];

            // 检查是否与上一个添加的元素是同一位置（合并重复项）
            // _Values.size() > _RowPtrs[currentRow] 确保我们只检查当前行内的元素
            if (_Values.size() > _RowPtrs[currentRow] && _ColIndices.back() == trip.Col) {
                _Values.back() = trip.Value;
            } else {
                _Values.push_back(trip.Value);
                _ColIndices.push_back(trip.Col);
            }
        }
    }
    _RowPtrs[_Rows] = static_cast<int>(_Values.size());
}

void SparseMatrix::SetFromTriplets(const std::vector<Triplet> &triplets) {
    _Triplets = triplets;
    SetFromTriplets();
}

cv::Mat SparseMatrix::operator*(const cv::Mat &other) const {
    if (other.rows != _Cols || other.cols != 1) {
        SuperDebug::Error("SparseMatrix Multiply dimension mismatch: Matrix [{}x{}] vs Vector [{}x{}]",
                          _Rows, _Cols, other.rows, other.cols);
        return cv::Mat();
    }

    if (other.type() != CV_64F) {
        SuperDebug::Error("SparseMatrix Multiply type mismatch: Input vector must be CV_64F");
        return cv::Mat();
    }

    cv::Mat result(_Rows, 1, CV_64F);

    const double *otherPtr = other.ptr<double>();
    double *resultPtr = result.ptr<double>();

#pragma omp parallel for
    for (int i = 0; i < _Rows; ++i) {
        double sum = 0.0;
        for (int idx = _RowPtrs[i]; idx < _RowPtrs[i + 1]; ++idx) {
            sum += _Values[idx] * otherPtr[_ColIndices[idx]];
        }
        resultPtr[i] = sum;
    }

    return result;
}

} // namespace RSPIP::Math::LinearSystem