#pragma once
#include "SuperDebug.hpp"
#include <opencv2/opencv.hpp>

namespace RSPIP {

// Represents image data + metadata (geospatial or simple image)
class ImageData {
  public:
    std::vector<cv::Mat> BandDatas; // Pixel data(BGRA bands stored separately)
    std::string ImageName;
    std::vector<std::string> DataType; // e.g. "uint8", "float32"
    std::string Projection;            // CRS (for GeoTIFF)
    std::vector<double> GeoTransform;  // Affine transform (6 elements)

    int Height() const { return static_cast<int>(BandDatas[0].rows); }
    int Width() const { return static_cast<int>(BandDatas[0].cols); }
    int GetBandCounts() const { return static_cast<int>(BandDatas.size()); }

    void MergeDataFromBands() {
        if (BandDatas.empty()) {
            Error("No band data to merge.");
            throw std::runtime_error("No band data to merge.");
        }
        cv::merge(BandDatas, _MergedData);
    }

    cv::Mat &GetMergedData() {
        MergeDataFromBands();
        return _MergedData;
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

        return BandDatas[band - 1].at<uchar>(row, col);
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

        BandDatas[band - 1].at<uchar>(row, col) = value;
    }

    cv::Vec3b GetPixelValue(int row, int col) {
        if (_IsOutOfBounds(row, col)) {
            Error("Pixel position out of range: ({}, {})", row, col);
            throw std::runtime_error("Pixel position or Band out of range.");
        }

        return GetMergedData().at<cv::Vec3b>(row, col);
    }

    void SetPixelValue(int row, int col, cv::Vec3b value) {
        if (_IsOutOfBounds(row, col)) {
            Error("Pixel position out of range: ({}, {})", row, col);
            throw std::runtime_error("Pixel position or Band out of range.");
        }

        _MergedData.at<cv::Vec3b>(row, col) = value;

        for (size_t b = 0; b < GetBandCounts(); ++b) {
            BandDatas[b].at<uchar>(row, col) =
                _MergedData.at<cv::Vec3b>(row, col)[b];
        }
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

    int GetImageType(int band = 1) const {
        if (BandDatas.empty())
            return -1;
        return BandDatas[band].type();
    }

  private:
    bool _IsOutOfBounds(size_t row, size_t col) const {
        if (BandDatas.empty()) {
            Error("Image data is empty.");
            return true;
        }

        return row >= static_cast<size_t>(BandDatas[0].rows) ||
               col >= static_cast<size_t>(BandDatas[0].cols);
    }
    cv::Mat _MergedData;
};

} // namespace RSPIP
