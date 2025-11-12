// Create By GuoShuai
// Idea From YuXiaoYu
// A flexible multi-temporal orthoimage mosaicking method based on dynamic
// variable patches

#pragma once
#include "IMosaicAlgorithm.h"
#include "Util/SortImage.hpp"
#include "Util/SuperDebug.hpp"

namespace RSPIP::MosaicAlgorithm {

class DynmiacPatchMosaic : public IMosaicAlgorithm {
  public:
    void
    MosaicImages(std::vector<std::shared_ptr<GeoImage>> &ImageDatas) override {
        _InitMosaicParameters(ImageDatas);
        Info("Mosaic Image Size: {} x {}", _MosaicCols, _MosaicRows);

        RSPIP::Util::SortImagesByLongitude(ImageDatas);

        for (const auto &imgData : ImageDatas) {
            _PasteImageToMosaic(imgData);
        }

        Info("Mosaic Completed.");
    }

  private:
    void _PasteImageToMosaic(const std::shared_ptr<GeoImage> &imgData) {
        const auto &gt = imgData->GeoTransform;
        int width = imgData->Width();
        int height = imgData->Height();

        Info("Pasting Image: {} Siez: {} x {}", imgData->ImageName, width,
             height);

        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {

                auto pixelValue = imgData->GetPixelValue(row, col);
                if (pixelValue == imgData->NonData) {
                    continue;
                }

                double lon = gt[0] + col * gt[1] + row * gt[2];
                double lat = gt[3] + col * gt[4] + row * gt[5];

                auto mosaicX = static_cast<int>(std::lround(
                    (lon - _GeoBounds.minLongtitude) * _InversePixelSizeX));
                auto mosaicY = static_cast<int>(std::lround(
                    (lat - _GeoBounds.minLatitude) * _InversePixelSizeY));

                if (mosaicX < 0 || mosaicX >= _MosaicCols || mosaicY < 0 ||
                    mosaicY >= _MosaicRows) {
                    std::cerr << "Warning: Mosaic position out of bounds ("
                              << mosaicX << ", " << mosaicY << "). Skipping."
                              << std::endl;
                    continue;
                }
                MosaicResult->SetPixelValue(mosaicY, mosaicX, pixelValue);
            }

            std::cout << "\rPasting Image: "
                      << static_cast<int>((row + 1) * 100.0 / height) << "%"
                      << std::flush;
        }
    }
};
} // namespace RSPIP::MosaicAlgorithm
