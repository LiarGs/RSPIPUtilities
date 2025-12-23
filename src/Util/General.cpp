#include "Util/General.h"
#include "Util/SuperDebug.hpp"
#include "gdal_priv.h"
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace RSPIP::Util {

double BGRToGray(const cv::Vec3b &pixelValue) {
    return 0.299 * pixelValue[2] + 0.587 * pixelValue[1] + 0.114 * pixelValue[0];
}

bool IsGeoImage(const std::string &imagePath) {
    auto ext = std::filesystem::path(imagePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == ".tif" || ext == ".tiff");
}

GDALDataType CVTypeToGDALType(int cvType) {
    // 获取深度信息，忽略通道数
    int depth = CV_MAT_DEPTH(cvType);

    switch (depth) {
    case CV_8U:
        return GDT_Byte;
    case CV_16U:
        return GDT_UInt16;
    case CV_16S:
        return GDT_Int16;
    case CV_32S:
        return GDT_Int32;
    case CV_32F:
        return GDT_Float32;
    case CV_64F:
        return GDT_Float64;
    default:
        Error("Unknown OpenCV data type depth: {}", depth);
        return GDT_Byte;
    }
}

int GDALTypeToCVType(GDALDataType gdalType, int channels) {
    switch (gdalType) {
    case GDT_Byte:
        return CV_MAKETYPE(CV_8U, channels);
    case GDT_UInt16:
        return CV_MAKETYPE(CV_16U, channels);
    case GDT_Int16:
        return CV_MAKETYPE(CV_16S, channels);
    case GDT_Int32:
        return CV_MAKETYPE(CV_32S, channels);
    case GDT_Float32:
        return CV_MAKETYPE(CV_32F, channels);
    case GDT_Float64:
        return CV_MAKETYPE(CV_64F, channels);
    case GDT_CInt16:
    case GDT_CInt32:
    case GDT_CFloat32:
    case GDT_CFloat64:
        Error("GDAL complex data types are not fully supported.");
        return -1;
    default:
        Error("Unknown GDAL data type: {}", GDALGetDataTypeName(gdalType));
        return CV_MAKETYPE(CV_8U, channels);
    }
}

std::vector<std::string> GetTifImagePathFromPath(const std::string &path) {
    std::vector<std::string> imagePaths;
    if (!std::filesystem::exists(path)) {
        Error("Path does not exist: {}", path);
        return imagePaths;
    }

    for (const auto &entry : std::filesystem::directory_iterator(path)) {
        if (!entry.is_regular_file()) {
            continue;
        }
        auto filePath = entry.path().string();
        if (IsGeoImage(filePath)) {
            imagePaths.push_back(filePath);
        }
    }
    return imagePaths;
}

} // namespace RSPIP::Util