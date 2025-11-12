#pragma once
#include "Image.hpp"
#include "Util/SuperDebug.hpp"
#include "gdal_priv.h"

namespace RSPIP {

// Represents image data + metadata (geospatial or simple image)
class GeoImage : public Image {
  public:
    std::vector<cv::Mat> ExtraBandDatas; // Extra band data
    std::string Projection;              // CRS (for GeoTIFF)
    std::vector<double> GeoTransform;    // Affine transform (6 elements)
    cv::Vec3b NonData;

    GeoImage() : Projection(""), GeoTransform(6, 0.0), NonData(0, 0, 0) {}

    int GetBandCounts() const override {
        return static_cast<int>(ExtraBandDatas.size() + ImageData.channels());
    }

    using Image::SetPixelValue; // 避免隐藏掉另一个重载函数
    void SetPixelValue(int row, int col, int band, uchar value) override {
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
            ImageData.at<cv::Vec3b>(row, col)[band - 1] = value;
        } else {
            ExtraBandDatas[band - 4].at<uchar>(row, col) = value;
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

    void PrintImageInfo() override {
        Info("Image Name: {} Dimensions: {} x {}", ImageName, Width(),
             Height());
        Info("Number of Bands: {}", GetBandCounts());
        // GeoInfo
        if (!Projection.empty()) {
            Info("Projection:\n {}", Projection);
        }
        if (GeoTransform.size() == 6) {
            Info("GeoTransform:\n [{}, {}, {}, {}, {}, {}]", GeoTransform[0],
                 GeoTransform[1], GeoTransform[2], GeoTransform[3],
                 GeoTransform[4], GeoTransform[5]);
        }
        Info("Latitude/Longitude of Top-Left Pixel (0,0): ({}, {})",
             GetLatitude(0, 0), GetLongitude(0, 0));
    }
};

} // namespace RSPIP
