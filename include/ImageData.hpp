#pragma once
#include <opencv2/opencv.hpp>

namespace RSPIP {

// Represents image data + metadata (geospatial or simple image)
class ImageData {
  public:
    std::vector<cv::Mat> Data; // Pixel data(BGRA bands stored separately)
    std::string ImageName;
    std::vector<std::string> DataType; // e.g. "uint8", "float32"
    std::string Projection;            // CRS (for GeoTIFF)
    std::vector<double> GeoTransform;  // Affine transform (6 elements)

    int Height() const { return static_cast<int>(Data[0].rows); }
    int Width() const { return static_cast<int>(Data[0].cols); }
    int GetBandCounts() const { return static_cast<int>(Data.size()); }

    int GetImageType(int band = 1) const {
        if (Data.empty())
            return -1;
        return Data[band].type();
    }

    cv::Mat GetMergedData() {
        if (_MergeData.empty()) {
            cv::merge(Data, _MergeData);
        }
        return _MergeData;
    }

  private:
    cv::Mat _MergeData;
};

} // namespace RSPIP
