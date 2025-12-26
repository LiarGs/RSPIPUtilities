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

void SparseMatrix::SetFromTriplets(const std::vector<Triplet> &triplets) {
    _Values.clear();
    _ColIndices.clear();
    _RowPtrs.clear();

    std::vector<Triplet> sortedTriplets = triplets;
    std::sort(sortedTriplets.begin(), sortedTriplets.end(), [](const Triplet &a, const Triplet &b) {
        if (a.Row != b.Row)
            return a.Row < b.Row;
        return a.Col < b.Col;
    });

    // 构建 CSR
    _RowPtrs.resize(_Rows + 1, 0);
    _Values.reserve(sortedTriplets.size());
    _ColIndices.reserve(sortedTriplets.size());

    int currentTripIdx = 0;
    for (int currentRow = 0; currentRow < _Rows; ++currentRow) {
        _RowPtrs[currentRow] = static_cast<int>(_Values.size());

        while (currentTripIdx < sortedTriplets.size() && sortedTriplets[currentTripIdx].Row == currentRow) {
            const auto &trip = sortedTriplets[currentTripIdx++];
            _Values.push_back(trip.Value);
            _ColIndices.push_back(trip.Col);
        }
    }
    _RowPtrs[_Rows] = static_cast<int>(_Values.size());
}

cv::Mat SparseMatrix::operator*(const cv::Mat &other) const {
    if (other.rows != _Cols || other.cols != 1) {
        SuperDebug::Error("SparseMatrix Multiply dimension mismatch: Matrix [{}x{}] vs Vector [{}x{}]",
                          _Rows, _Cols, other.rows, other.cols);
        return cv::Mat();
    }
    cv::Mat result(other.rows, 1, CV_64F);

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