#pragma once
#include "Basic/GeoInfo.h"
#include <opencv2/core.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace RSPIP {

class Image {
  public:
    Image();
    Image(const cv::Mat &imageData);
    Image(const cv::Mat &imageData, const std::string &imageName);
    ~Image() = default;

    int Height() const;
    int Width() const;

    int GetBandCounts() const;

    template <typename T>
    T GetPixelValue(int row, int col) const {
        IsOutOfBounds(row, col);
        return ImageData.at<T>(static_cast<int>(row), static_cast<int>(col));
    }

    template <typename T>
    T GetPixelValue(int row, int col, int band) const {
        if (IsOutOfBounds(row, col, band))
            return -1;

        const T *ptr = ImageData.ptr<T>(static_cast<int>(row));
        return ptr[col * ImageData.channels() + (band - 1)];
    }

    template <typename T>
    void SetPixelValue(int row, int col, T value) {
        if (IsOutOfBounds(row, col)) {
            return;
        }
        ImageData.at<T>(row, col) = value;
    }

    template <typename T>
    void SetPixelValue(int row, int col, int band, T value) {
        if (IsOutOfBounds(row, col, band))
            return;

        T *ptr = ImageData.ptr<T>(row);
        ptr[col * ImageData.channels() + (band - 1)] = value;
    }

    int GetImageType() const;

    bool IsOutOfBounds(int row, int col) const;
    bool IsOutOfBounds(int row, int col, int band) const;
    bool HasNoData() const;
    bool IsNoDataPixel(int row, int col) const;
    cv::Scalar GetFillScalarForOpenCV() const;
    bool IsBgrColorImage() const;

  public:
    cv::Mat ImageData;
    std::string ImageName;
    std::vector<double> NoDataValues;
    std::optional<GeoInfo> GeoInfo;
};

} // namespace RSPIP
