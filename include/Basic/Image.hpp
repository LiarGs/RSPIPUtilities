#pragma once
#include "Interface/IImageVisitor.hpp"
#include "Util/SuperDebug.hpp"
#include <opencv2/opencv.hpp>

namespace RSPIP {
class Image {
  public:
    Image() : ImageData(), ImageName("Default.png") {}
    Image(const cv::Mat &imageData) : ImageData(imageData), ImageName("Default.png") {}
    Image(const cv::Mat &imageData, const std::string &imageName) : ImageData(imageData), ImageName(imageName) {}

    virtual void Accept(IImageVisitor &visitor) const {
        visitor.Visit(*this);
    }

    int Height() const { return ImageData.rows; }
    int Width() const { return ImageData.cols; }

    virtual int GetBandCounts() const { return ImageData.channels(); }

    template <typename T>
    T GetPixelValue(size_t row, size_t col) const {
        _IsOutOfBounds(row, col);

        return ImageData.at<T>(row, col);
    }

    virtual uchar GetPixelValue(size_t row, size_t col, int band) const {
        _IsOutOfBounds(row, col, band);

        const uchar *ptr = ImageData.ptr<uchar>(row);
        return ptr[col * ImageData.channels() + (band - 1)];
    }

    template <typename T>
    void SetPixelValue(int row, int col, T value) {
        if (_IsOutOfBounds(row, col)) {
            return;
        }

        ImageData.at<T>(row, col) = value;
    }

    virtual void SetPixelValue(int row, int col, int band, uchar value) {
        if (_IsOutOfBounds(row, col, band)) {
            return;
        }

        uchar *ptr = ImageData.ptr<uchar>(row);
        ptr[col * ImageData.channels() + (band - 1)] = value;
    }

    int GetImageType() const {
        if (ImageData.empty())
            return -1;
        return ImageData.type();
    }

  protected:
    bool _IsOutOfBounds(int row, int col) const {
        if (ImageData.empty()) {
            Error("ImageData is empty.");
            throw std::runtime_error("ImageData is empty.");
            return true;
        } else if (row < 0 || row >= Height() || col < 0 || col >= Width()) {
            Error("Pixel position out of range: ({}, {})", row, col);
            throw std::runtime_error("Pixel position or Band out of range.");
            return true;
        }

        return false;
    }

    bool _IsOutOfBounds(int row, int col, int band) const {
        if (_IsOutOfBounds(row, col)) {
            return true;
        } else if (band <= 0 || band > GetBandCounts()) {
            Error("Band index out of range: {} (valid range: 1-{})", band, GetBandCounts());
            throw std::out_of_range("Band index out of range");
            return true;
        }

        return false;
    }

  public:
    cv::Mat ImageData; // Pixel data (BGRA merged)
    std::string ImageName;
};
} // namespace RSPIP