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
        MosaicResult = std::make_shared<GeoImage>();
        _GetGeoInfo(basicMosaicParam->ImageDatas);
        _GetDimensions(basicMosaicParam->ImageDatas);
    }

    virtual void _PasteImageToMosaicResult(const std::shared_ptr<GeoImage> &imageData) {

        int columns = imageData->Width();
        int rows = imageData->Height();

        Info("Pasting Image: {} Size: {} x {}", imageData->ImageName, rows, columns);

#pragma omp parallel for
        for (int row = 0; row < rows; ++row) {
            for (int col = 0; col < columns; ++col) {

                auto pixelValue = imageData->GetPixelValue<cv::Vec3b>(row, col);
                if (pixelValue == imageData->NonData) {
                    continue;
                }

                auto [latitude, longitude] = imageData->GetLatLon(row, col);
                auto [mosaicRow, mosaicColumn] = MosaicResult->LatLonToRC(latitude, longitude);

                if (mosaicRow == -1 || mosaicColumn == -1) {
                    continue;
                }

                MosaicResult->SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
            }
        }
    }

  private:
    void _GetGeoInfo(const std::vector<std::shared_ptr<GeoImage>> &ImageDatas) {
        MosaicResult->ImageBounds.MinLongitude = MosaicResult->ImageBounds.MinLatitude = std::numeric_limits<double>::max();
        MosaicResult->ImageBounds.MaxLongitude = MosaicResult->ImageBounds.MaxLatitude = std::numeric_limits<double>::lowest();

        for (const auto &img : ImageDatas) {
            MosaicResult->ImageBounds.MinLongitude = std::min(MosaicResult->ImageBounds.MinLongitude, img->ImageBounds.MinLongitude);
            MosaicResult->ImageBounds.MaxLongitude = std::max(MosaicResult->ImageBounds.MaxLongitude, img->ImageBounds.MaxLongitude);
            MosaicResult->ImageBounds.MinLatitude = std::min(MosaicResult->ImageBounds.MinLatitude, img->ImageBounds.MinLatitude);
            MosaicResult->ImageBounds.MaxLatitude = std::max(MosaicResult->ImageBounds.MaxLatitude, img->ImageBounds.MaxLatitude);
        }

        MosaicResult->GeoTransform = {MosaicResult->ImageBounds.MinLongitude, ImageDatas[0]->GeoTransform[1], 0,
                                      MosaicResult->ImageBounds.MaxLatitude, 0, ImageDatas[0]->GeoTransform[5]};

        MosaicResult->Projection = ImageDatas[0]->Projection;
    }

    void _GetDimensions(const std::vector<std::shared_ptr<GeoImage>> &ImageDatas) {
        const auto &gt = ImageDatas[0]->GeoTransform;
        const auto mosaicCols = (MosaicResult->ImageBounds.MaxLongitude - MosaicResult->ImageBounds.MinLongitude) / gt[1];
        const auto mosaicRows = (MosaicResult->ImageBounds.MaxLatitude - MosaicResult->ImageBounds.MinLatitude) / std::abs(gt[5]);

        MosaicResult->ImageData = cv::Mat(mosaicRows + 1, mosaicCols + 1, CV_8UC3, MosaicResult->NonData);
    }

  public:
    virtual ~MosaicAlgorithmBase() = default;

    std::shared_ptr<GeoImage> MosaicResult;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm