#include "Algorithm/Mosaic/Simple.h"
#include "Util/SuperDebug.hpp"

namespace RSPIP::Algorithm::MosaicAlgorithm {

Simple::Simple(std::vector<Image> imageDatas) : MosaicAlgorithmBase(std::move(imageDatas)) {}

void Simple::Execute() {
    SuperDebug::Info("Mosaic Image Size: {} x {}", AlgorithmResult.Height(), AlgorithmResult.Width());

    for (const auto &imageData : _MosaicImages) {
        _PasteImageToMosaicResult(imageData);
    }

    SuperDebug::Info("Mosaic Completed.");
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm
