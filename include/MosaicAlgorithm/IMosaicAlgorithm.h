#pragma once

#include "Basic/GeoImage.hpp"
#include <vector>

namespace RSPIP::MosaicAlgorithm {

struct ImageBounds {
    double minLongtitude, maxLongtitude, minLatitude, maxLatitude;
};

class IMosaicAlgorithm {
  public:
    virtual ~IMosaicAlgorithm() = default;

    virtual void
    MosaicImages(std::vector<std::shared_ptr<GeoImage>> &ImageDatas) = 0;

    std::shared_ptr<GeoImage> MosaicResult;

  protected:
    void _InitMosaicParameters(
        const std::vector<std::shared_ptr<GeoImage>> &ImageDatas) {
        _GetGeoBounds(ImageDatas);
        _GetPixelSize(ImageDatas);
        _GetDimiensions(ImageDatas);
    }

    ImageBounds _GeoBounds;
    double _InversePixelSizeX, _InversePixelSizeY;
    int _MosaicRows, _MosaicCols;

  private:
    void
    _GetGeoBounds(const std::vector<std::shared_ptr<GeoImage>> &ImageDatas) {
        _GeoBounds.minLongtitude = _GeoBounds.minLatitude =
            std::numeric_limits<double>::max();
        _GeoBounds.maxLongtitude = _GeoBounds.maxLatitude =
            std::numeric_limits<double>::lowest();

        for (const auto &img : ImageDatas) {
            const auto &gt = img->GeoTransform;
            int width = img->Width();
            int height = img->Height();

            double cornersX[4];
            double cornersY[4];
            for (int i = 0; i < 4; ++i) {
                int px = (i == 1 || i == 2) ? width : 0;
                int py = (i >= 2) ? height : 0;
                cornersX[i] = gt[0] + px * gt[1] + py * gt[2];
                cornersY[i] = gt[3] + px * gt[4] + py * gt[5];
            }

            for (int i = 0; i < 4; ++i) {
                _GeoBounds.minLongtitude =
                    std::min(_GeoBounds.minLongtitude, cornersX[i]);
                _GeoBounds.maxLongtitude =
                    std::max(_GeoBounds.maxLongtitude, cornersX[i]);
                _GeoBounds.minLatitude =
                    std::min(_GeoBounds.minLatitude, cornersY[i]);
                _GeoBounds.maxLatitude =
                    std::max(_GeoBounds.maxLatitude, cornersY[i]);
            }
        }
    }

    void
    _GetPixelSize(const std::vector<std::shared_ptr<GeoImage>> &ImageDatas) {
        if (ImageDatas.empty()) {
            throw std::runtime_error("No images provided for pixel size.");
        }
        const auto &gt = ImageDatas[0]->GeoTransform;
        _InversePixelSizeX = 1 / gt[1];
        _InversePixelSizeY = 1 / std::abs(gt[5]);
    }

    void
    _GetDimiensions(const std::vector<std::shared_ptr<GeoImage>> &ImageDatas) {
        _MosaicCols = (_GeoBounds.maxLongtitude - _GeoBounds.minLongtitude) *
                      _InversePixelSizeX;
        _MosaicRows = (_GeoBounds.maxLatitude - _GeoBounds.minLatitude) *
                      _InversePixelSizeY;

        MosaicResult = std::make_shared<GeoImage>();
        MosaicResult->ImageData = cv::Mat(_MosaicRows + 1, _MosaicCols + 1,
                                          CV_8UC3, MosaicResult->NonData);
    }
};

} // namespace RSPIP::MosaicAlgorithm