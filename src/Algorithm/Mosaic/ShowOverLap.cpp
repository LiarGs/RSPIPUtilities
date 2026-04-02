#include "Algorithm/Mosaic/ShowOverLap.h"
#include "Util/Color.h"
#include "Util/SuperDebug.hpp"
#include <omp.h>

namespace RSPIP::Algorithm::MosaicAlgorithm {

ShowOverLap::ShowOverLap(std::vector<Image> imageDatas) : MosaicAlgorithmBase(std::move(imageDatas)) {}

void ShowOverLap::Execute() {
    SuperDebug::Info("Mosaic Image Size: {} x {}", AlgorithmResult.Height(), AlgorithmResult.Width());

    for (const auto &imageData : _MosaicImages) {
        _PasteImageToMosaicResult(imageData);
    }

    SuperDebug::Info("Mosaic Completed.");
}

void ShowOverLap::_PasteImageToMosaicResult(const Image &imageData) {
    if (!imageData.GeoInfo.has_value() || !AlgorithmResult.GeoInfo.has_value()) {
        return;
    }

    const int columns = imageData.Width();
    const int rows = imageData.Height();

#pragma omp parallel for
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            const auto &pixelValue = imageData.GetPixelValue<cv::Vec3b>(row, col);
            if (imageData.IsNoDataPixel(row, col)) {
                continue;
            }

            const auto [latitude, longitude] = imageData.GeoInfo->GetLatLon(row, col);
            const auto [mosaicRow, mosaicColumn] = AlgorithmResult.GeoInfo->LatLonToRC(latitude, longitude);
            if (mosaicRow == -1 || mosaicColumn == -1) {
                continue;
            }

            if (AlgorithmResult.IsNoDataPixel(mosaicRow, mosaicColumn)) {
                AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
            } else {
                AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, Color::Red);
            }
        }
    }
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm
