#pragma once
#include "SuperDebug.hpp"
#include "gdal_priv.h"
#include <opencv2/opencv.hpp>

namespace RSPIP {

// Represents image data + metadata (geospatial or simple image)
class ImageData {
  public:
    GDALDataset *dataset;
    cv::Mat BGRData;                     // Pixel data (BGRA merged)
    std::vector<cv::Mat> ExtraBandDatas; // Extra band data
    std::string ImageName;
    std::string Projection;           // CRS (for GeoTIFF)
    std::vector<double> GeoTransform; // Affine transform (6 elements)
    cv::Vec3b NonData;

    ImageData()
        : dataset(nullptr), ImageName("Defalut.png"), Projection(""),
          GeoTransform(6, 0.0), NonData(0, 0, 0) {}

    int Height() const { return static_cast<int>(BGRData.rows); }
    int Width() const { return static_cast<int>(BGRData.cols); }
    int GetBandCounts() const {
        return static_cast<int>(ExtraBandDatas.size() + 3);
    }

    uchar GetPixelValue(int row, int col, int band) const {
        if (band >= GetBandCounts()) {
            Error("Band index out of range: {}", band);
            throw std::runtime_error("Band index out of range: " +
                                     std::to_string(band));
        }

        if (_IsOutOfBounds(row, col)) {
            Error("Pixel position out of range: ({}, {})", row, col);
            throw std::runtime_error("Pixel position or Band out of range.");
        }

        return BGRData.at<cv::Vec3b>(row, col)[band - 1];
    }

    void SetPixelValue(int row, int col, int band, uchar value) {
        if (band >= GetBandCounts()) {
            Error("Band index out of range: {}", band);
            throw std::runtime_error("Band index out of range: " +
                                     std::to_string(band));
        }

        if (_IsOutOfBounds(row, col)) {
            Error("Pixel position out of range: ({}, {})", row, col);
            throw std::runtime_error("Pixel position or Band out of range.");
        }

        if (band <= 3) {
            BGRData.at<cv::Vec3b>(row, col)[band - 1] = value;
        } else {
            ExtraBandDatas[band - 4].at<uchar>(row, col) = value;
        }
    }

    cv::Vec3b GetPixelValue(int row, int col) {
        if (_IsOutOfBounds(row, col)) {
            Error("Pixel position out of range: ({}, {})", row, col);
            throw std::runtime_error("Pixel position or Band out of range.");
        }

        return BGRData.at<cv::Vec3b>(row, col);
    }

    void SetPixelValue(int row, int col, cv::Vec3b value) {
        if (_IsOutOfBounds(row, col)) {
            Error("Pixel position out of range: ({}, {})", row, col);
            throw std::runtime_error("Pixel position or Band out of range.");
        }

        BGRData.at<cv::Vec3b>(row, col) = value;
    }

    // (0, 0) at top-left corner
    double GetLongitude(size_t positionX, size_t positionY) const {
        if (GeoTransform.size() != 6)
            throw std::runtime_error("GeoTransform data is not available.");
        return GeoTransform[0] + positionX * GeoTransform[1] +
               positionY * GeoTransform[2];
    }

    // (0, 0) at top-left corner
    double GetLatitude(size_t positionX, size_t positionY) const {
        if (GeoTransform.size() != 6)
            throw std::runtime_error("GeoTransform data is not available.");
        return GeoTransform[3] + positionX * GeoTransform[4] +
               positionY * GeoTransform[5];
    }

    int GetImageType() const {
        if (BGRData.empty())
            return -1;
        return BGRData.type();
    }

  private:
    bool _IsOutOfBounds(size_t row, size_t col) const {
        if (BGRData.empty()) {
            Error("Image data is empty.");
            return true;
        }

        return row >= Height() || col >= Width();
    }
};

} // namespace RSPIP
