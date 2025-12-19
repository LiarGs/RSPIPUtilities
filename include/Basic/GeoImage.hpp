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
    GeoImage(const cv::Mat &imageData) : Image(imageData), ExtraBandDatas(), NonData(Color::Black) {}
    GeoImage(const cv::Mat &imageData, const std::string &imageName) : Image(imageData, imageName), ExtraBandDatas(), NonData(Color::Black) {}

    int GetBandCounts() const override { return static_cast<int>(ExtraBandDatas.size() + ImageData.channels()); }

    void Accept(IImageVisitor &visitor) const override {
        visitor.Visit(*this);
    }

    using Image::GetPixelValue;
    uchar GetPixelValue(size_t row, size_t col, int band) const override {
        if (_IsOutOfBounds(row, col, band)) {
            return 0;
        }

        int baseChannels = ImageData.channels();
        if (band <= baseChannels) {
            const uchar *ptr = ImageData.ptr<uchar>(row);
            return ptr[col * baseChannels + (band - 1)];
        } else {
            return ExtraBandDatas[band - baseChannels - 1].at<uchar>(row, col);
        }
    }

    using Image::SetPixelValue;
    void SetPixelValue(int row, int col, int band, uchar value) override {
        _IsOutOfBounds(row, col, band);

        int baseChannels = ImageData.channels();
        if (band <= baseChannels) {
            ImageData.ptr<uchar>(row)[col * baseChannels + (band - 1)] = value;
        } else {
            ExtraBandDatas[band - baseChannels - 1].at<uchar>(row, col) = value;
        }
    }

  public:
    std::vector<cv::Mat> ExtraBandDatas;
    cv::Vec3b NonData;
};

} // namespace RSPIP
