#pragma once
#include "Image.hpp"
#include "Interface/IGeoTransformer.hpp"
#include "Util/Color.hpp"
#include "gdal_priv.h"

namespace RSPIP {

// Represents image data + metadata (geospatial or simple image)
class GeoImage : public Image, public IGeoTransformer {
  public:
    GeoImage() : Image(), ExtraBandDatas(), NonData(Color::Black) {}

    int GetBandCounts() const override { return static_cast<int>(ExtraBandDatas.size() + ImageData.channels()); }

    using Image::SetPixelValue; // 避免隐藏掉另一个重载函数
    void SetPixelValue(int row, int col, int band, uchar value) override {
        if (band >= GetBandCounts()) {
            Error("Band index out of range: {}", band);
            throw std::runtime_error("Band index out of range: " + std::to_string(band));
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

    void PrintImageInfo() override {
        Image::PrintImageInfo();
        IGeoTransformer::PrintGeoInfo();
    }

  public:
    std::vector<cv::Mat> ExtraBandDatas; // Extra band data
    cv::Vec3b NonData;
};

} // namespace RSPIP
