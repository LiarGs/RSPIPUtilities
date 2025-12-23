#pragma once
#include "gdal.h"
#include <opencv2/core/cvdef.h>
#include <opencv2/core/types.hpp>
#include <string>
#include <vector>

namespace RSPIP::Util {

enum class ImageType {
    NormalImage,
    GeoImage,
    MaskImage,
    CloudMask
};

double BGRToGray(const cv::Vec3b &pixelValue);

bool IsGeoImage(const std::string &imagePath);

GDALDataType CVTypeToGDALType(int cvType);

int GDALTypeToCVType(GDALDataType gdalType, int channels = 1);

std::vector<std::string> GetTifImagePathFromPath(const std::string &path);

} // namespace RSPIP::Util