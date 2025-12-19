#pragma once
#include "Util/SuperDebug.hpp"

namespace RSPIP {

struct ImageGeoBounds {
  public:
    double MinLongitude, MaxLongitude, MinLatitude, MaxLatitude;
};

class IGeoTransformer {
  public:
    // (0, 0) at top-left corner
    virtual double GetLongitude(size_t row, size_t column) const {
        if (GeoTransform.size() != 6) {
            Error("GeoTransform data is not available.");
        }

        return GeoTransform[0] + column * GeoTransform[1] + row * GeoTransform[2];
    }

    // (0, 0) at top-left corner
    virtual double GetLatitude(size_t row, size_t column) const {
        if (GeoTransform.size() != 6) {
            Error("GeoTransform data is not available.");
        }

        return GeoTransform[3] + column * GeoTransform[4] + row * GeoTransform[5];
    }

    // (0, 0) at top-left corner
    virtual std::pair<double, double> GetLatLon(size_t row, size_t column) const {
        if (GeoTransform.size() != 6) {
            Error("GeoTransform data is not available.");
        }

        return {GetLatitude(row, column), GetLongitude(row, column)};
    }

    virtual std::pair<int, int> LatLonToRC(double latitude, double longitude) const {
        if (GeoTransform.size() != 6) {
            Error("GeoTransform data is not available.");

            return {-1, -1};
        }

        if (!IsContain(latitude, longitude)) {
            Error("Warning: Mosaic position out of bounds ({}, {}). Skipping", latitude, longitude);

            return {-1, -1};
        }

        auto row = static_cast<int>(std::lround((ImageBounds.MaxLatitude - latitude) / std::abs(GeoTransform[5])));
        auto column = static_cast<int>(std::lround((longitude - ImageBounds.MinLongitude) / GeoTransform[1]));

        return {row, column};
    }

    virtual bool IsContain(double lat, double lon) const {
        return lat >= ImageBounds.MinLatitude && lat <= ImageBounds.MaxLatitude &&
               lon >= ImageBounds.MinLongitude && lon <= ImageBounds.MaxLongitude;
    }

    void PrintGeoInfo() const {
        if (!Projection.empty()) {
            Info("Projection:\n {}", Projection);
        } else {
            Error("Projection data is not available.");
        }
        if (GeoTransform.size() == 6) {
            Info("GeoTransform:\n [{}, {}, {}, {}, {}, {}]",
                 GeoTransform[0], GeoTransform[1],
                 GeoTransform[2], GeoTransform[3],
                 GeoTransform[4], GeoTransform[5]);
        } else {
            Error("GeoTransform data is not available.");
        }

        Info("Image Bounds: ({}, {}) - ({}, {})",
             ImageBounds.MinLatitude, ImageBounds.MinLongitude,
             ImageBounds.MaxLatitude, ImageBounds.MaxLongitude);
    }

  public:
    std::vector<double> GeoTransform; // 分别是经度、像素宽度、旋转、纬度、旋转、像素高度
    std::string Projection;           // CRS (for GeoTIFF)
    ImageGeoBounds ImageBounds;

  protected:
    IGeoTransformer() = default;
    IGeoTransformer(const IGeoTransformer &) = default;
    IGeoTransformer(IGeoTransformer &&) = default;
    IGeoTransformer &operator=(const IGeoTransformer &) = default;
    IGeoTransformer &operator=(IGeoTransformer &&) = default;
};
} // namespace RSPIP