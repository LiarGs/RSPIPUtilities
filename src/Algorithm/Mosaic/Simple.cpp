#include "Algorithm/Mosaic/Simple.h"
#include "Util/SuperDebug.hpp"

namespace RSPIP::Algorithm::MosaicAlgorithm {

Simple::Simple(const std::vector<GeoImage> &imageDatas) : MosaicAlgorithmBase(imageDatas) {}

void Simple::Execute() {
    SuperDebug::Info("Mosaic Image Size: {} x {}", AlgorithmResult.Height(), AlgorithmResult.Width());

    for (const auto &imgData : _MosaicImages) {
        _PasteImageToMosaicResult(imgData);
    }

    SuperDebug::Info("Mosaic Completed.");
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm