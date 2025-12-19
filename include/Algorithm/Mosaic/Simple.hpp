#pragma once
#include "MosaicAlgorithmBase.hpp"
#include "Util/SuperDebug.hpp"

namespace RSPIP::Algorithm::MosaicAlgorithm {

class Simple : public MosaicAlgorithmBase {
  public:
    Simple(const std::vector<GeoImage> &imageDatas) : MosaicAlgorithmBase(imageDatas) {}

    void Execute() override {
        _Mosaic();
    }

  private:
    void _Mosaic() {

        Info("Mosaic Image Size: {} x {}", AlgorithmResult.Height(), AlgorithmResult.Width());

        for (const auto &imgData : _MosaicImages) {
            _PasteImageToMosaicResult(imgData);
        }

        Info("Mosaic Completed.");
    }
};
} // namespace RSPIP::Algorithm::MosaicAlgorithm
