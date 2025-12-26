#include "Algorithm/Mosaic/DynamicPatch.h"
#include "Math/Statistics.hpp"
#include "Util/General.h"
#include "Util/SortImage.hpp"
#include "Util/SuperDebug.hpp"

namespace RSPIP::Algorithm::MosaicAlgorithm {

DynamicPatch::DynamicPatch(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks)
    : MosaicAlgorithmBase(imageDatas),
      _ImageDatas(imageDatas), // 注意：基类存的是引用，这里 _ImageDatas 是拷贝
      _CloudMasks(cloudMasks),
      _CurrentProcessImageIndex(0) {}

void DynamicPatch::Execute() {
    RSPIP::Util::SortImagesByLongitude(_ImageDatas);
    RSPIP::Util::SortImagesByLongitude(_CloudMasks);
    for (size_t index = 0; index < _ImageDatas.size(); ++index) {
        _CloudMasks[index].Init();
        _CloudMasks[index].SetSourceImage(_ImageDatas[index]);
    }

    SuperDebug::Info("Mosaic Image Size: {} x {}", AlgorithmResult.Height(), AlgorithmResult.Width());

    for (size_t currentImageIndex = 0; currentImageIndex < _ImageDatas.size(); ++currentImageIndex) {
        _CurrentProcessImageIndex = currentImageIndex;
        _PasteImageToMosaicResult(_ImageDatas[_CurrentProcessImageIndex]);
        _ProcessWithClouds();
    }

    SuperDebug::Info("Mosaic Completed.");
}

void DynamicPatch::_PasteImageToMosaicResult(const GeoImage &imageData) {
    auto columns = imageData.Width();
    auto rows = imageData.Height();

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

            if (AlgorithmResult.GetPixelValue<cv::Vec3b>(mosaicRow, mosaicColumn) != AlgorithmResult.NonData) {
                continue;
            }

            AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
        }
    }
}

void DynamicPatch::_ProcessWithClouds() {
    const auto &cloudMask = _CloudMasks[_CurrentProcessImageIndex];

#pragma omp parallel for
    for (int cloudGroupIndex = 0; cloudGroupIndex < cloudMask.CloudGroups.size(); ++cloudGroupIndex) {
        // Info("Processing Cloud Group: {}/{}", cloudGroupIndex + 1, cloudMask.CloudGroups.size());

        _ProcessWithCloudGroup(cloudMask.CloudGroups[cloudGroupIndex]);
    }
}

void DynamicPatch::_ProcessWithCloudGroup(const CloudGroup &cloudGroup) {
    for (const auto &[_, rowCloudPixels] : cloudGroup.CloudPixelMap) {
        for (size_t cloudPixelIndex = 0; cloudPixelIndex < rowCloudPixels.size(); ++cloudPixelIndex) {
            const auto &cloudPixel = rowCloudPixels[cloudPixelIndex];

            auto [latitude, longitude] = std::pair{cloudPixel.Latitude, cloudPixel.Longitude};
            auto [mosaicRow, mosaicColumn] = AlgorithmResult.LatLonToRC(latitude, longitude);

            if (mosaicRow == -1 || mosaicColumn == -1) {
                continue;
            }

            if (AlgorithmResult.GetPixelValue<cv::Vec3b>(mosaicRow, mosaicColumn)[0] != cloudPixel.Value) {
                // 此处云已被填充
                continue;
            }

            const auto &bestPatch = _OptimalPatchSelection(rowCloudPixels, cloudPixelIndex);

            if (bestPatch.empty()) {
                continue;
            }

            for (const auto &pixel : bestPatch) {
                auto [mosaicResultPixelRow, mosaicResultPixelCol] = AlgorithmResult.LatLonToRC(pixel.Latitude, pixel.Longitude);
                AlgorithmResult.SetPixelValue(mosaicResultPixelRow, mosaicResultPixelCol, pixel.Value);
            }
        }
    }
}

std::vector<GeoPixel<cv::Vec3b>> DynamicPatch::_OptimalPatchSelection(const std::vector<CloudPixel<unsigned char>> &rowCloudPixels, size_t currentCloudPixelIndex) {
    double bestPearsonCorrelation = -1.0;
    std::vector<GeoPixel<cv::Vec3b>> bestPatch = {};

    for (size_t otherImageIndex = 0; otherImageIndex < _ImageDatas.size(); ++otherImageIndex) {
        if (otherImageIndex == _CurrentProcessImageIndex) {
            continue;
        }
        const auto &otherImage = _ImageDatas[otherImageIndex];

        std::vector<double> _mosaicResultPreviousLine = {};
        std::vector<double> _otherImagePreviousLine = {};
        std::vector<GeoPixel<cv::Vec3b>> _currentPatch = {};

        // 收集上一行对应的数据
        for (size_t cloudPixelIndex = currentCloudPixelIndex; cloudPixelIndex < rowCloudPixels.size(); ++cloudPixelIndex) {

            const auto &nextCloudPixel = rowCloudPixels[cloudPixelIndex];
            if (!otherImage.IsContain(nextCloudPixel.Latitude, nextCloudPixel.Longitude)) {
                break;
            }

            auto [otherImagePixelRow, otherImagePixelCol] = otherImage.LatLonToRC(nextCloudPixel.Latitude, nextCloudPixel.Longitude);
            if (otherImagePixelRow == 0) {
                break;
            }

            auto previousLinepixelValue = otherImage.GetPixelValue<cv::Vec3b>(otherImagePixelRow - 1, otherImagePixelCol);
            if (previousLinepixelValue == otherImage.NonData) {
                break;
            }

            _otherImagePreviousLine.emplace_back(Util::BGRToGray(previousLinepixelValue));

            auto [mosaicResultPixelRow, mosaicResultPixelCol] = AlgorithmResult.LatLonToRC(nextCloudPixel.Latitude, nextCloudPixel.Longitude);
            _mosaicResultPreviousLine.emplace_back(Util::BGRToGray(AlgorithmResult.GetPixelValue<cv::Vec3b>(mosaicResultPixelRow - 1, mosaicResultPixelCol)));

            auto currentPatchPixelValue = otherImage.GetPixelValue<cv::Vec3b>(otherImagePixelRow, otherImagePixelCol);
            _currentPatch.emplace_back(currentPatchPixelValue, otherImagePixelRow, otherImagePixelCol, nextCloudPixel.Latitude, nextCloudPixel.Longitude);
        }

        if (_mosaicResultPreviousLine.empty() || _otherImagePreviousLine.empty()) {
            continue;
        }
        auto pearsonCorrelation = Math::PearsonCorrelation(_mosaicResultPreviousLine, _otherImagePreviousLine);

        if (pearsonCorrelation > bestPearsonCorrelation) {
            bestPatch = std::move(_currentPatch);
            bestPearsonCorrelation = pearsonCorrelation;
        }
    }

    return bestPatch;
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm