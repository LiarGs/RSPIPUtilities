#pragma once
#include "Util/SuperDebug.hpp"
#include <opencv2/opencv.hpp>

namespace RSPIP {
class Image {
  public:
    Image() : ImageData(), ImageName("Default.png") {}
    Image(const cv::Mat &imageData) : ImageData(imageData), ImageName("Default.png") {}
    Image(const cv::Mat &imageData, const std::string &imageName) : ImageData(imageData), ImageName(imageName) {}

    int Height() const { return ImageData.rows; }
    int Width() const { return ImageData.cols; }

    virtual int GetBandCounts() const { return ImageData.channels(); }

    template <typename T>
    T GetPixelValue(size_t row, size_t col) const {
        if (_IsOutOfBounds(row, col)) {
            Error("Pixel position out of range: ({}, {}) when GetPixelValue", row, col);
        }

        return ImageData.at<T>(row, col);
    }

    virtual uchar GetPixelValue(size_t row, size_t col, int band) const {
        if (band >= GetBandCounts()) {
            Error("Band index out of range: {} (valid range: 0-{})", band, GetBandCounts() - 1);
            throw std::out_of_range("Band index out of range");
        }

        if (_IsOutOfBounds(row, col)) {
            Error("Pixel position out of range: ({}, {})", row, col);
            throw std::out_of_range("Pixel position or Band out of range.");
        }

        return ImageData.at<cv::Vec3b>(row, col)[band - 1];
    }

    template <typename T>
    void SetPixelValue(int row, int col, T value) {
        if (_IsOutOfBounds(row, col)) {
            Error("Pixel position out of range: ({}, {}) when SetPixelValue", row, col);
        }

        ImageData.at<T>(row, col) = value;
    }

    virtual void SetPixelValue(int row, int col, int band, uchar value) {
        if (band >= GetBandCounts()) {
            Error("Band index out of range: {} (valid range: 0-{})", band, GetBandCounts() - 1);
            throw std::out_of_range("Band index out of range");
        }

        if (_IsOutOfBounds(row, col)) {
            Error("Pixel position out of range: ({}, {})", row, col);
            throw std::runtime_error("Pixel position or Band out of range.");
        }

        ImageData.at<cv::Vec3b>(row, col)[band - 1] = value;
    }

    int GetImageType() const {
        if (ImageData.empty())
            return -1;
        return ImageData.type();
    }

    virtual void PrintImageInfo() {
        Info("Image Name: {} Dimensions: Rows: {} x Cols: {}", ImageName, Height(), Width());
        Info("Number of Bands: {}", GetBandCounts());
    }

  protected:
    bool _IsOutOfBounds(size_t row, size_t col) const {
        if (ImageData.empty()) {
            Error("ImageData is empty.");
            throw std::runtime_error("ImageData is empty.");
            return true;
        }
        return row >= Height() || col >= Width();
    }

  public:
    cv::Mat ImageData; // Pixel data (BGRA merged)
    std::string ImageName;
};
} // namespace RSPIP