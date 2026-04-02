#include "Basic/GeoInfo.h"
#include "Util/SuperDebug.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace RSPIP {

namespace {

constexpr double kAffineEpsilon = 1e-9;

std::pair<double, double> _ApplyGeoTransform(const std::vector<double> &geoTransform, double row, double column) {
    return {
        geoTransform[3] + column * geoTransform[4] + row * geoTransform[5],
        geoTransform[0] + column * geoTransform[1] + row * geoTransform[2]};
}

} // namespace

bool GeoInfo::HasValidTransform() const {
    return GeoTransform.size() == 6;
}

double GeoInfo::GetLongitude(size_t row, size_t column) const {
    if (!HasValidTransform()) {
        SuperDebug::Error("GeoTransform data is not available.");
        return 0.0;
    }

    return _ApplyGeoTransform(GeoTransform, static_cast<double>(row), static_cast<double>(column)).second;
}

double GeoInfo::GetLatitude(size_t row, size_t column) const {
    if (!HasValidTransform()) {
        SuperDebug::Error("GeoTransform data is not available.");
        return 0.0;
    }

    return _ApplyGeoTransform(GeoTransform, static_cast<double>(row), static_cast<double>(column)).first;
}

std::pair<double, double> GeoInfo::GetLatLon(size_t row, size_t column) const {
    if (!HasValidTransform()) {
        SuperDebug::Error("GeoTransform data is not available.");
        return {0.0, 0.0};
    }

    return _ApplyGeoTransform(GeoTransform, static_cast<double>(row), static_cast<double>(column));
}

bool GeoInfo::TryWorldToPixel(double latitude, double longitude, double &row, double &column) const {
    if (!HasValidTransform()) {
        return false;
    }

    const double determinant = GeoTransform[1] * GeoTransform[5] - GeoTransform[2] * GeoTransform[4];
    if (std::abs(determinant) < kAffineEpsilon) {
        return false;
    }

    const double deltaX = longitude - GeoTransform[0];
    const double deltaY = latitude - GeoTransform[3];
    column = (deltaX * GeoTransform[5] - deltaY * GeoTransform[2]) / determinant;
    row = (deltaY * GeoTransform[1] - deltaX * GeoTransform[4]) / determinant;
    return true;
}

std::pair<int, int> GeoInfo::LatLonToRC(double latitude, double longitude) const {
    if (!HasValidTransform() || !Contains(latitude, longitude)) {
        return {-1, -1};
    }

    double row = 0.0;
    double column = 0.0;
    if (!TryWorldToPixel(latitude, longitude, row, column)) {
        return {-1, -1};
    }
    return {static_cast<int>(std::lround(row)), static_cast<int>(std::lround(column))};
}

bool GeoInfo::Contains(double latitude, double longitude) const {
    return latitude >= Bounds.MinLatitude && latitude <= Bounds.MaxLatitude &&
           longitude >= Bounds.MinLongitude && longitude <= Bounds.MaxLongitude;
}

void GeoInfo::RebuildBounds(int rows, int columns) {
    if (!HasValidTransform() || rows <= 0 || columns <= 0) {
        Bounds = {};
        return;
    }

    const double maxRow = static_cast<double>(std::max(rows - 1, 0));
    const double maxColumn = static_cast<double>(std::max(columns - 1, 0));
    const auto corners = std::array<std::pair<double, double>, 4>{
        _ApplyGeoTransform(GeoTransform, 0.0, 0.0),
        _ApplyGeoTransform(GeoTransform, 0.0, maxColumn),
        _ApplyGeoTransform(GeoTransform, maxRow, 0.0),
        _ApplyGeoTransform(GeoTransform, maxRow, maxColumn)};

    Bounds = {
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::lowest()};

    for (const auto &[latitude, longitude] : corners) {
        Bounds.MinLongitude = std::min(Bounds.MinLongitude, longitude);
        Bounds.MaxLongitude = std::max(Bounds.MaxLongitude, longitude);
        Bounds.MinLatitude = std::min(Bounds.MinLatitude, latitude);
        Bounds.MaxLatitude = std::max(Bounds.MaxLatitude, latitude);
    }
}

void GeoInfo::PrintGeoInfo() const {
    if (!Projection.empty()) {
        SuperDebug::Info("Projection:\n {}", Projection);
    } else {
        SuperDebug::Error("Projection data is not available.");
    }

    if (HasValidTransform()) {
        SuperDebug::Info("GeoTransform:\n [{}, {}, {}, {}, {}, {}]",
                         GeoTransform[0], GeoTransform[1],
                         GeoTransform[2], GeoTransform[3],
                         GeoTransform[4], GeoTransform[5]);
    } else {
        SuperDebug::Error("GeoTransform data is not available.");
    }

    SuperDebug::Info("Image Bounds: ({}, {}) - ({}, {})",
                     Bounds.MinLatitude, Bounds.MinLongitude,
                     Bounds.MaxLatitude, Bounds.MaxLongitude);
}

} // namespace RSPIP
