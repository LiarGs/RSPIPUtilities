#include "Util/ImageInfoPrinter.h"
#include "Util/SuperDebug.hpp"

namespace RSPIP::Util {

void PrintImageInfo(const Image &image) {
    SuperDebug::Info("Image Name: {} Dimensions: Rows: {} x Cols: {}", image.ImageName, image.Height(), image.Width());
    SuperDebug::Info("Number of Bands: {}", image.GetBandCounts());

    if (image.GeoInfo.has_value()) {
        image.GeoInfo->PrintGeoInfo();
    }
}

} // namespace RSPIP::Util
