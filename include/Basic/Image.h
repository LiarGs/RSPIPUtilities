#pragma once
#include <opencv2/core/mat.hpp>
#include <stdexcept>
#include <string>

namespace RSPIP {

class IImageVisitor;

class Image {
  public:
    Image();
    Image(const cv::Mat &imageData);
    Image(const cv::Mat &imageData, const std::string &imageName);
    virtual ~Image() = default;

    virtual void Accept(IImageVisitor &visitor) const;

    int Height() const;
    int Width() const;

    virtual int GetBandCounts() const;

    template <typename T>
    T GetPixelValue(int row, int col) const {
        _IsOutOfBounds(row, col);
        return ImageData.at<T>(static_cast<int>(row), static_cast<int>(col));
    }

    template <typename T>
    T GetPixelValue(int row, int col, int band) const {
        if (_IsOutOfBounds(row, col, band))
            return -1;

        const T *ptr = ImageData.ptr<T>(static_cast<int>(row));
        return ptr[col * ImageData.channels() + (band - 1)];
    }

    template <typename T>
    void SetPixelValue(int row, int col, T value) {
        if (_IsOutOfBounds(row, col)) {
            return;
        }
        ImageData.at<T>(row, col) = value;
    }

    template <typename T>
    void SetPixelValue(int row, int col, int band, T value) {
        if (_IsOutOfBounds(row, col, band))
            return;

        T *ptr = ImageData.ptr<T>(row);
        ptr[col * ImageData.channels() + (band - 1)] = value;
    }

    int GetImageType() const;

  protected:
    bool _IsOutOfBounds(int row, int col) const;
    bool _IsOutOfBounds(int row, int col, int band) const;

  public:
    cv::Mat ImageData;
    std::string ImageName;
};
} // namespace RSPIP