#pragma once
#include <string>
#include <utility>
#include <vector>

namespace RSPIP {

struct ImageGeoBounds {
  public:
    double MinLongitude = 0.0;
    double MaxLongitude = 0.0;
    double MinLatitude = 0.0;
    double MaxLatitude = 0.0;
};

struct GeoInfo {
  public:
    bool HasValidTransform() const;
    double GetLongitude(size_t row, size_t column) const;
    double GetLatitude(size_t row, size_t column) const;
    std::pair<double, double> GetLatLon(size_t row, size_t column) const;
    bool TryWorldToPixel(double latitude, double longitude, double &row, double &column) const;
    std::pair<int, int> LatLonToRC(double latitude, double longitude) const;
    bool Contains(double latitude, double longitude) const;
    void RebuildBounds(int rows, int columns);
    void PrintGeoInfo() const;

  public:
    std::vector<double> GeoTransform;
    std::string Projection;
    ImageGeoBounds Bounds;
};

} // namespace RSPIP
