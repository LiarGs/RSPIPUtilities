#include "Interface/IGeoTransformer.h"
#include "Util/SuperDebug.hpp"
#include <cmath>
#include <iostream>

namespace RSPIP {

double IGeoTransformer::GetLongitude(size_t row, size_t column) const {
    if (GeoTransform.size() != 6) {
        SuperDebug::Error("GeoTransform data is not available.");
        return 0.0;
    }

    return GeoTransform[0] + column * GeoTransform[1] + row * GeoTransform[2];
}

double IGeoTransformer::GetLatitude(size_t row, size_t column) const {
    if (GeoTransform.size() != 6) {
        SuperDebug::Error("GeoTransform data is not available.");
        return 0.0;
    }

    return GeoTransform[3] + column * GeoTransform[4] + row * GeoTransform[5];
}

std::pair<double, double> IGeoTransformer::GetLatLon(size_t row, size_t column) const {
    if (GeoTransform.size() != 6) {
        SuperDebug::Error("GeoTransform data is not available.");
        return {0.0, 0.0};
    }

    return {GetLatitude(row, column), GetLongitude(row, column)};
}

std::pair<int, int> IGeoTransformer::LatLonToRC(double latitude, double longitude) const {
    if (GeoTransform.size() != 6) {
        SuperDebug::Error("GeoTransform data is not available.");
        return {-1, -1};
    }

    if (!IsContain(latitude, longitude)) {
        SuperDebug::Warn("Warning: Mosaic position out of bounds ({}, {}). Skipping", latitude, longitude);
        return {-1, -1};
    }

    auto row = static_cast<int>(std::lround((ImageBounds.MaxLatitude - latitude) / std::abs(GeoTransform[5])));
    auto column = static_cast<int>(std::lround((longitude - ImageBounds.MinLongitude) / GeoTransform[1]));

    return {row, column};
}

bool IGeoTransformer::IsContain(double lat, double lon) const {
    return lat >= ImageBounds.MinLatitude && lat <= ImageBounds.MaxLatitude &&
           lon >= ImageBounds.MinLongitude && lon <= ImageBounds.MaxLongitude;
}

void IGeoTransformer::PrintGeoInfo() const {
    if (!Projection.empty()) {
        SuperDebug::Info("Projection:\n {}", Projection);
    } else {
        SuperDebug::Error("Projection data is not available.");
    }
    if (GeoTransform.size() == 6) {
        SuperDebug::Info("GeoTransform:\n [{}, {}, {}, {}, {}, {}]",
                         GeoTransform[0], GeoTransform[1],
                         GeoTransform[2], GeoTransform[3],
                         GeoTransform[4], GeoTransform[5]);
    } else {
        SuperDebug::Error("GeoTransform data is not available.");
    }

    SuperDebug::Info("Image Bounds: ({}, {}) - ({}, {})",
                     ImageBounds.MinLatitude, ImageBounds.MinLongitude,
                     ImageBounds.MaxLatitude, ImageBounds.MaxLongitude);
}

} // namespace RSPIP