#pragma once
#include "MosaicAlgorithmBase.hpp"
#include "Util/SuperDebug.hpp"

namespace RSPIP::Algorithm::MosaicAlgorithm {

class SimpleMosaic : public MosaicAlgorithmBase {
  public:
    void Execute(std::shared_ptr<AlgorithmParamBase> params) override {
        if (auto basicMosaicParams = std::dynamic_pointer_cast<BasicMosaicParam>(params)) {
            _InitMosaicParameters(basicMosaicParams);
            _MosaicImages(basicMosaicParams->ImageDatas);
        }
    }

  private:
    void _MosaicImages(std::vector<std::shared_ptr<GeoImage>> &imageDatas) {

        Info("Mosaic Image Size: {} x {}", MosaicResult->Height(), MosaicResult->Width());

        for (const auto &imgData : imageDatas) {
            _PasteImageToMosaicResult(imgData);
        }

        Info("Mosaic Completed.");
    }
};
} // namespace RSPIP::Algorithm::MosaicAlgorithm
