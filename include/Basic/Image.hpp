#pragma once
#include "Util/SuperDebug.hpp"
#include <opencv2/opencv.hpp>

namespace RSPIP {
class Image {
  public:
    Image() : ImageData(), ImageName("Default.png") {}
    int Height() const { return static_cast<int>(ImageData.rows); }
    int Width() const { return static_cast<int>(ImageData.cols); }

    virtual int GetBandCounts() const { return ImageData.channels(); }

    virtual cv::Vec3b GetPixelValue(int row, int col) {
        if (_IsOutOfBounds(row, col)) {
            Error("Pixel position out of range: ({}, {})", row, col);
            throw std::runtime_error("Pixel position or Band out of range.");
        }

        return ImageData.at<cv::Vec3b>(row, col);
    }

    virtual uchar GetPixelValue(int row, int col, int band) const {
        if (band >= GetBandCounts()) {
            Error("Band index out of range: {}", band);
            throw std::runtime_error("Band index out of range: " +
                                     std::to_string(band));
        }

        if (_IsOutOfBounds(row, col)) {
            Error("Pixel position out of range: ({}, {})", row, col);
            throw std::runtime_error("Pixel position or Band out of range.");
        }

        return ImageData.at<cv::Vec3b>(row, col)[band - 1];
    }

    virtual void SetPixelValue(int row, int col, cv::Vec3b value) {
        if (_IsOutOfBounds(row, col)) {
            Error("Pixel position out of range: ({}, {})", row, col);
            throw std::runtime_error("Pixel position or Band out of range.");
        }

        ImageData.at<cv::Vec3b>(row, col) = value;
    }

    virtual void SetPixelValue(int row, int col, int band, uchar value) {
        if (band >= GetBandCounts()) {
            Error("Band index out of range: {}", band);
            throw std::runtime_error("Band index out of range: " +
                                     std::to_string(band));
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
        Info("Image Name: {} Dimensions: {} x {}", ImageName, Width(),
             Height());
        Info("Number of Bands: {}", GetBandCounts());
    }

  protected:
    bool _IsOutOfBounds(size_t row, size_t col) const {
        if (ImageData.empty()) {
            Error("Image data is empty.");
            return true;
        }

        return row >= Height() || col >= Width();
    }

  public:
    cv::Mat ImageData; // Pixel data (BGRA merged)
    std::string ImageName;
};
} // namespace RSPIP