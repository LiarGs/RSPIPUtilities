#include "Algorithm/Mosaic/ShowOverLap.h"
#include "Util/Color.h"
#include "Util/SuperDebug.hpp"
#include <omp.h>

namespace RSPIP::Algorithm::MosaicAlgorithm {

ShowOverLap::ShowOverLap(const std::vector<GeoImage> &imageDatas) : MosaicAlgorithmBase(imageDatas) {}

void ShowOverLap::Execute() {
    SuperDebug::Info("Mosaic Image Size: {} x {}", AlgorithmResult.Height(), AlgorithmResult.Width());

    for (const auto &imgData : _MosaicImages) {
        _PasteImageToMosaicResult(imgData);
    }

    SuperDebug::Info("Mosaic Completed.");
}

void ShowOverLap::_PasteImageToMosaicResult(const GeoImage &imageData) {
    int columns = imageData.Width();
    int rows = imageData.Height();

    // Info("Pasting Image: {} Size: {} x {}", imageData.ImageName, rows, columns);

#pragma omp parallel for
    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {

            const auto &pixelValue = imageData.GetPixelValue<cv::Vec3b>(row, col);
            if (pixelValue == imageData.NonData) {
                continue;
            }

            auto [latitude, longitude] = imageData.GetLatLon(row, col);
            auto [mosaicRow, mosaicColumn] = AlgorithmResult.LatLonToRC(latitude, longitude);

            if (mosaicRow == -1 || mosaicColumn == -1) {
                continue;
            }
            if (AlgorithmResult.GetPixelValue<cv::Vec3b>(mosaicRow, mosaicColumn) == AlgorithmResult.NonData) {
                AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
            } else {
                AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, Color::Red);
            }
        }
    }
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm