#pragma once
#include "Basic/GeoImage.hpp"
#include "MosaicAlgorithmBase.hpp"
#include "Util/Color.hpp"

namespace RSPIP::Algorithm::MosaicAlgorithm {

class ShowOverLap : public MosaicAlgorithmBase {
  public:
    void Execute(std::shared_ptr<AlgorithmParamBase> params) override {
        if (auto basicMosaicParams = std::dynamic_pointer_cast<BasicMosaicParam>(params)) {
            _InitMosaicParameters(basicMosaicParams);
            _MosaicImages(basicMosaicParams->ImageDatas);
        }
    }

  private:
    void _MosaicImages(std::vector<std::shared_ptr<GeoImage>> &imageDatas) {
        Info("Mosaic Image Size: {} x {}", AlgorithmResult->Height(), AlgorithmResult->Width());

        for (const auto &imgData : imageDatas) {
            _PasteImageToMosaicResult(imgData);
        }

        Info("Mosaic Completed.");
    }

    void _PasteImageToMosaicResult(const std::shared_ptr<const GeoImage> &imageData) override {
        int columns = imageData->Width();
        int rows = imageData->Height();

        Info("Pasting Image: {} Size: {} x {}", imageData->ImageName, rows, columns);

#pragma omp parallel for
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < columns; ++col) {

                const auto &pixelValue = imageData->GetPixelValue<cv::Vec3b>(row, col);
                if (pixelValue == imageData->NonData) {
                    continue;
                }

                auto [latitude, longitude] = imageData->GetLatLon(row, col);
                auto [mosaicRow, mosaicColumn] = AlgorithmResult->LatLonToRC(latitude, longitude);

                if (mosaicRow == -1 || mosaicColumn == -1) {
                    continue;
                }
                if (AlgorithmResult->GetPixelValue<cv::Vec3b>(mosaicRow, mosaicColumn) == AlgorithmResult->NonData) {
                    AlgorithmResult->SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
                } else {
                    AlgorithmResult->SetPixelValue(mosaicRow, mosaicColumn, Color::Red);
                }
            }
        }
    }
};
} // namespace RSPIP::Algorithm::MosaicAlgorithm
