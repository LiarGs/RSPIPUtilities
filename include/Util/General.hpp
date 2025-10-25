#pragma once
#include "SuperDebug.hpp"
#include "gdal_priv.h"

// TODO: More complete type mapping
inline int GDALTypeToCVType(GDALDataType gdalType, int channels = 1) {
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