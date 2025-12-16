#pragma once
#include "Basic/GeoImage.hpp"
#include "Interface/IAlgorithm.h"

namespace RSPIP::Algorithm::MosaicAlgorithm {

struct BasicMosaicParam : AlgorithmParamBase {
  public:
    BasicMosaicParam(const std::vector<std::shared_ptr<GeoImage>> &imageDatas) : ImageDatas(imageDatas) {}

  public:
    std::vector<std::shared_ptr<GeoImage>> ImageDatas;
};

class MosaicAlgorithmBase : public IAlgorithm {
  protected:
    MosaicAlgorithmBase() = default;
    MosaicAlgorithmBase(const MosaicAlgorithmBase &) = default;
    MosaicAlgorithmBase(MosaicAlgorithmBase &&) = default;

    virtual void _InitMosaicParameters(const std::shared_ptr<BasicMosaicParam> &basicMosaicParam) {
        AlgorithmResult = std::make_shared<GeoImage>();
        _GetGeoInfo(basicMosaicParam->ImageDatas);
        _GetDimensions(basicMosaicParam->ImageDatas);
    }

    virtual void _PasteImageToMosaicResult(const std::shared_ptr<const GeoImage> &imageData) {

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

                AlgorithmResult->SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
            }
        }
    }

  private:
    void _GetGeoInfo(const std::vector<std::shared_ptr<GeoImage>> &ImageDatas) {
        AlgorithmResult->ImageBounds.MinLongitude = AlgorithmResult->ImageBounds.MinLatitude = std::numeric_limits<double>::max();
        AlgorithmResult->ImageBounds.MaxLongitude = AlgorithmResult->ImageBounds.MaxLatitude = std::numeric_limits<double>::lowest();

        for (const auto &img : ImageDatas) {
            AlgorithmResult->ImageBounds.MinLongitude = std::min(AlgorithmResult->ImageBounds.MinLongitude, img->ImageBounds.MinLongitude);
            AlgorithmResult->ImageBounds.MaxLongitude = std::max(AlgorithmResult->ImageBounds.MaxLongitude, img->ImageBounds.MaxLongitude);
            AlgorithmResult->ImageBounds.MinLatitude = std::min(AlgorithmResult->ImageBounds.MinLatitude, img->ImageBounds.MinLatitude);
            AlgorithmResult->ImageBounds.MaxLatitude = std::max(AlgorithmResult->ImageBounds.MaxLatitude, img->ImageBounds.MaxLatitude);
        }

        AlgorithmResult->GeoTransform = {AlgorithmResult->ImageBounds.MinLongitude, ImageDatas[0]->GeoTransform[1], 0,
                                         AlgorithmResult->ImageBounds.MaxLatitude, 0, ImageDatas[0]->GeoTransform[5]};

        AlgorithmResult->Projection = ImageDatas[0]->Projection;
    }

    void _GetDimensions(const std::vector<std::shared_ptr<GeoImage>> &ImageDatas) {
        const auto &gt = ImageDatas[0]->GeoTransform;
        const auto mosaicCols = (AlgorithmResult->ImageBounds.MaxLongitude - AlgorithmResult->ImageBounds.MinLongitude) / gt[1];
        const auto mosaicRows = (AlgorithmResult->ImageBounds.MaxLatitude - AlgorithmResult->ImageBounds.MinLatitude) / std::abs(gt[5]);

        AlgorithmResult->ImageData = cv::Mat(mosaicRows + 1, mosaicCols + 1, CV_8UC3, AlgorithmResult->NonData);
    }

  public:
    virtual ~MosaicAlgorithmBase() = default;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm