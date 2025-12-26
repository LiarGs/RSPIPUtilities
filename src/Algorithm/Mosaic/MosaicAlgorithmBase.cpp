#include "Algorithm/Mosaic/MosaicAlgorithmBase.h"
#include "Util/SuperDebug.hpp"
#include <cmath>
#include <limits>
#include <omp.h>

namespace RSPIP::Algorithm::MosaicAlgorithm {

MosaicAlgorithmBase::MosaicAlgorithmBase(const std::vector<GeoImage> &imageDatas)
    : AlgorithmResult(), _MosaicImages(imageDatas) {
    _InitMosaicParameters();
}

void MosaicAlgorithmBase::_InitMosaicParameters() {
    _GetGeoInfo();
    _GetDimensions();
}

void MosaicAlgorithmBase::_GetGeoInfo() {
    AlgorithmResult.ImageBounds.MinLongitude = std::numeric_limits<double>::max();
    AlgorithmResult.ImageBounds.MinLatitude = std::numeric_limits<double>::max();
    AlgorithmResult.ImageBounds.MaxLongitude = std::numeric_limits<double>::lowest();
    AlgorithmResult.ImageBounds.MaxLatitude = std::numeric_limits<double>::lowest();

    for (const auto &img : _MosaicImages) {
        AlgorithmResult.ImageBounds.MinLongitude = std::min(AlgorithmResult.ImageBounds.MinLongitude, img.ImageBounds.MinLongitude);
        AlgorithmResult.ImageBounds.MaxLongitude = std::max(AlgorithmResult.ImageBounds.MaxLongitude, img.ImageBounds.MaxLongitude);
        AlgorithmResult.ImageBounds.MinLatitude = std::min(AlgorithmResult.ImageBounds.MinLatitude, img.ImageBounds.MinLatitude);
        AlgorithmResult.ImageBounds.MaxLatitude = std::max(AlgorithmResult.ImageBounds.MaxLatitude, img.ImageBounds.MaxLatitude);
    }

    // 镶嵌结果继承第一张图的分辨率和旋转参数
    AlgorithmResult.GeoTransform = {AlgorithmResult.ImageBounds.MinLongitude, _MosaicImages[0].GeoTransform[1], 0,
                                    AlgorithmResult.ImageBounds.MaxLatitude, 0, _MosaicImages[0].GeoTransform[5]};

    AlgorithmResult.Projection = _MosaicImages[0].Projection;
}

void MosaicAlgorithmBase::_GetDimensions() {
    const auto &gt = AlgorithmResult.GeoTransform;
    // 防止除以0
    if (std::abs(gt[1]) < 1e-9 || std::abs(gt[5]) < 1e-9) {
        SuperDebug::Error("Invalid GeoTransform resolution");
        return;
    }

    const auto mosaicCols = static_cast<int>((AlgorithmResult.ImageBounds.MaxLongitude - AlgorithmResult.ImageBounds.MinLongitude) / gt[1]);
    const auto mosaicRows = static_cast<int>((AlgorithmResult.ImageBounds.MaxLatitude - AlgorithmResult.ImageBounds.MinLatitude) / std::abs(gt[5]));

    AlgorithmResult.NonData = Color::Black;
    AlgorithmResult.ImageData = cv::Mat(mosaicRows + 1, mosaicCols + 1, CV_8UC3, AlgorithmResult.NonData);
}

void MosaicAlgorithmBase::_PasteImageToMosaicResult(const GeoImage &imageData) {
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

            AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
        }
    }
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm