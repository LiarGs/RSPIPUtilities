#pragma once
#include "Util/SuperDebug.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace RSPIP::Math {

// 检查数据的长度是否一致，是否为空
template <typename T>
void ValidCheck(const std::vector<T> &data1, const std::vector<T> &data2) {
    if (data1.size() != data2.size()) {
        throw std::invalid_argument("数据长度不一致");
    }
    if (data1.empty() || data2.empty()) {
        throw std::invalid_argument("数据为空");
    }
}

// 计算数据的均值
template <typename T>
static double Mean(const std::vector<T> &data) {
    if (data.empty()) {
        Error("data is empty when calculate mean");
    }

    double sum = 0.0;
    for (const auto &value : data) {
        sum += static_cast<double>(value);
    }

    return sum / data.size();
}

// 计算数据的方差
template <typename T>
static double Variance(const std::vector<T> &data) {
    if (data.empty()) {
        Error("data is empty when calculate variance");
    }
    if (data.size() == 1) {
        return 0.0;
    }

    double mean = Mean(data);

    double sumSq = 0.0;
    for (const auto &value : data) {
        auto diff = static_cast<double>(value) - mean;
        sumSq += diff * diff;
    }

    return sumSq / (data.size() - 1);
}

// 计算数据的标准差
template <typename T>
static double StandardDeviation(const std::vector<T> &data) { return std::sqrt(Variance(data)); }

// 计算协方差
template <typename T>
static double Covariance(const std::vector<T> &data1, const std::vector<T> &data2) {
    ValidCheck(data1, data2);
    if (data1.size() == 1 || data2.size() == 1) {
        return 0.0;
    }

    int data1Size = data1.size();
    double mean1 = Mean(data1);
    double mean2 = Mean(data2);

    double covariance = 0.0;
    for (int i = 0; i < data1Size; i++) {
        covariance += (static_cast<double>(data1[i]) - mean1) *
                      (static_cast<double>(data2[i]) - mean2);
    }
    covariance /= data1Size - 1;

    return covariance;
}

// 计算两组数据的皮尔逊相关系数
template <typename T>
static double PearsonCorrelation(const std::vector<T> &data1, const std::vector<T> &data2) {
    ValidCheck(data1, data2);

    auto covariance = Covariance(data1, data2);
    auto standardDeviation1 = StandardDeviation(data1);
    auto standardDeviation2 = StandardDeviation(data2);

    return standardDeviation1 * standardDeviation2 == 0 ? 0.0 : covariance / (standardDeviation1 * standardDeviation2);
}

} // namespace RSPIP::Math