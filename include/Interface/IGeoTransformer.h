#pragma once
#include <string>
#include <utility>
#include <vector>

namespace RSPIP {

struct ImageGeoBounds {
  public:
    double MinLongitude, MaxLongitude, MinLatitude, MaxLatitude;
};

// (0, 0) at top-left corner
class IGeoTransformer {
  public:
    virtual ~IGeoTransformer() = default;

    virtual double GetLongitude(size_t row, size_t column) const;

    virtual double GetLatitude(size_t row, size_t column) const;

    virtual std::pair<double, double> GetLatLon(size_t row, size_t column) const;

    virtual std::pair<int, int> LatLonToRC(double latitude, double longitude) const;

    virtual bool IsContain(double lat, double lon) const;

    void PrintGeoInfo() const;

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