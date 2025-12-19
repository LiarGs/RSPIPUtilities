#pragma once
#include "Basic/GeoImage.hpp"
#include "Interface/IAlgorithm.h"

namespace RSPIP::Algorithm::MosaicAlgorithm {

class MosaicAlgorithmBase : public IAlgorithm {
  protected:
    MosaicAlgorithmBase() = default;
    MosaicAlgorithmBase(const std::vector<GeoImage> &imageDatas) : AlgorithmResult(), _MosaicImages(imageDatas) { _InitMosaicParameters(); }
    MosaicAlgorithmBase(const MosaicAlgorithmBase &) = default;
    MosaicAlgorithmBase(MosaicAlgorithmBase &&) = default;

    virtual void _PasteImageToMosaicResult(const GeoImage &imageData) {

        int columns = imageData.Width();
        int rows = imageData.Height();

        Info("Pasting Image: {} Size: {} x {}", imageData.ImageName, rows, columns);

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

  private:
    virtual void _InitMosaicParameters() {
        _GetGeoInfo();
        _GetDimensions();
    }

    void _GetGeoInfo() {
        AlgorithmResult.ImageBounds.MinLongitude = AlgorithmResult.ImageBounds.MinLatitude = std::numeric_limits<double>::max();
        AlgorithmResult.ImageBounds.MaxLongitude = AlgorithmResult.ImageBounds.MaxLatitude = std::numeric_limits<double>::lowest();

        for (const auto &img : _MosaicImages) {
            AlgorithmResult.ImageBounds.MinLongitude = std::min(AlgorithmResult.ImageBounds.MinLongitude, img.ImageBounds.MinLongitude);
            AlgorithmResult.ImageBounds.MaxLongitude = std::max(AlgorithmResult.ImageBounds.MaxLongitude, img.ImageBounds.MaxLongitude);
            AlgorithmResult.ImageBounds.MinLatitude = std::min(AlgorithmResult.ImageBounds.MinLatitude, img.ImageBounds.MinLatitude);
            AlgorithmResult.ImageBounds.MaxLatitude = std::max(AlgorithmResult.ImageBounds.MaxLatitude, img.ImageBounds.MaxLatitude);
        }

        AlgorithmResult.GeoTransform = {AlgorithmResult.ImageBounds.MinLongitude, _MosaicImages[0].GeoTransform[1], 0,
                                        AlgorithmResult.ImageBounds.MaxLatitude, 0, _MosaicImages[0].GeoTransform[5]};

        AlgorithmResult.Projection = _MosaicImages[0].Projection;
    }

    void _GetDimensions() {
        const auto &gt = _MosaicImages[0].GeoTransform;
        const auto mosaicCols = (AlgorithmResult.ImageBounds.MaxLongitude - AlgorithmResult.ImageBounds.MinLongitude) / gt[1];
        const auto mosaicRows = (AlgorithmResult.ImageBounds.MaxLatitude - AlgorithmResult.ImageBounds.MinLatitude) / std::abs(gt[5]);

        AlgorithmResult.ImageData = cv::Mat(mosaicRows + 1, mosaicCols + 1, CV_8UC3, AlgorithmResult.NonData);
    }

  public:
    virtual ~MosaicAlgorithmBase() = default;

  public:
    GeoImage AlgorithmResult;

  protected:
    const std::vector<GeoImage> &_MosaicImages;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm