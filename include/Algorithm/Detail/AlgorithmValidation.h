#pragma once
#include "Basic/Image.h"
#include "Util/SuperDebug.hpp"
#include <limits>
#include <string_view>

namespace RSPIP::Algorithm::Detail {

inline void ResetImageResult(Image &result) {
    result.ImageData.release();
    result.ImageName.clear();
    result.NoDataValues.clear();
    result.GeoInfo.reset();
}

inline void ResetEvaluationResult(double &value) {
    value = std::numeric_limits<double>::quiet_NaN();
}

inline bool ValidateNonEmpty(const Image &image, const char *algorithmName, std::string_view role) {
    if (!image.ImageData.empty()) {
        return true;
    }

    SuperDebug::Error("{} requires a non-empty {}.", algorithmName, role);
    return false;
}

inline bool ValidateSameSize(const Image &left, const Image &right, const char *algorithmName, std::string_view leftRole, std::string_view rightRole) {
    if (left.Height() == right.Height() && left.Width() == right.Width()) {
        return true;
    }

    SuperDebug::Error("{} requires {} and {} to have the same size. {}: {}x{}, {}: {}x{}",
                      algorithmName,
                      leftRole,
                      rightRole,
                      leftRole,
                      left.Width(),
                      left.Height(),
                      rightRole,
                      right.Width(),
                      right.Height());
    return false;
}

inline bool ValidateGeoImage(const Image &image, const char *algorithmName, std::string_view role) {
    if (image.GeoInfo.has_value() && image.GeoInfo->HasValidTransform()) {
        return true;
    }

    SuperDebug::Error("{} requires valid GeoInfo for {}: {}", algorithmName, role, image.ImageName);
    return false;
}

inline bool ValidateBgrImage(const Image &image, const char *algorithmName, std::string_view role) {
    if (image.IsBgrColorImage()) {
        return true;
    }

    SuperDebug::Error("{} requires 8-bit 3-channel BGR {}. Current type for {}: {}",
                      algorithmName,
                      role,
                      image.ImageName,
                      image.GetImageType());
    return false;
}

} // namespace RSPIP::Algorithm::Detail
