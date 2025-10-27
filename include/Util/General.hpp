#pragma once
#include "SuperDebug.hpp"
#include "gdal_priv.h"
#include < filesystem>

namespace RSPIP::Util {

inline bool IsGeoImage(const std::string &imagePath) {
    auto ext = std::filesystem::path(imagePath).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return (ext == ".tif" || ext == ".tiff");
}

inline GDALDataType CVTypeToGDALType(int cvType) {
    switch (cvType) {
    case CV_8UC1:
    case CV_8UC2:
    case CV_8UC3:
    case CV_8UC4:
        return GDT_Byte;
    case CV_16UC1:
    case CV_16UC2:
    case CV_16UC3:
    case CV_16UC4:
        return GDT_UInt16;
    case CV_16SC1:
    case CV_16SC2:
    case CV_16SC3:
    case CV_16SC4:
        return GDT_Int16;
    case CV_32SC1:
    case CV_32SC2:
    case CV_32SC3:
    case CV_32SC4:
        return GDT_Int32;
    case CV_32FC1:
    case CV_32FC2:
    case CV_32FC3:
    case CV_32FC4:
        return GDT_Float32;
    case CV_64FC1:
    case CV_64FC2:
    case CV_64FC3:
    case CV_64FC4:
        return GDT_Float64;
    default:
        Error("Unknown OpenCV data type: {}", cvType);
        return GDT_Byte;
    }
}

inline int GDALTypeToCVType(GDALDataType gdalType, int channels = 1) {
    // TODO: More complete type mapping
    switch (gdalType) {
    case GDT_Byte:
        return CV_8UC(channels);
    case GDT_UInt16:
        return CV_16UC(channels);
    case GDT_Int16:
        return CV_16SC(channels);
    case GDT_Int32:
        return CV_32SC(channels);
    case GDT_Float32:
        return CV_32FC(channels);
    case GDT_Float64:
        return CV_64FC(channels);
    case GDT_CInt16:
    case GDT_CInt32:
    case GDT_CFloat32:
    case GDT_CFloat64:
        Error("GDAL complex data types are not fully supported.");
        return -1; // TODO
    default:
        Error("Unknown GDAL data type: {}", GDALGetDataTypeName(gdalType));
        return CV_8UC(channels);
    }
}

} // namespace RSPIP::Util