#pragma once
#include <opencv2/core/mat.hpp> // 只包含必要的 OpenCV 头
#include <stdexcept>
#include <string>

namespace RSPIP {

// 前置声明，避免包含整个头文件
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
    T GetPixelValue(size_t row, size_t col) const {
        _IsOutOfBounds(row, col);
        return ImageData.at<T>(static_cast<int>(row), static_cast<int>(col));
    }

    virtual uchar GetPixelValue(size_t row, size_t col, int band) const;

    template <typename T>
    void SetPixelValue(int row, int col, T value) {
        if (_IsOutOfBounds(row, col)) {
            return;
        }
        ImageData.at<T>(row, col) = value;
    }

    virtual void SetPixelValue(int row, int col, int band, uchar value);

    int GetImageType() const;

  protected:
    bool _IsOutOfBounds(int row, int col) const;
    bool _IsOutOfBounds(int row, int col, int band) const;

  public:
    cv::Mat ImageData; // Pixel data
    std::string ImageName;
};
} // namespace RSPIP