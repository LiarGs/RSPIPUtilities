#include "Algorithm/Mosaic/AdaptiveIsophotePatch.h"
#include "Algorithm/Reconstruct/IsophoteConstrain.h"
#include "IO/ImageSaveVisitor.h"
#include "Math/Statistics.hpp"
#include "Util/General.h"
#include "Util/SortImage.hpp"
#include "Util/SuperDebug.hpp"

namespace RSPIP::Algorithm::MosaicAlgorithm {

AdaptiveIsophotePatch::AdaptiveIsophotePatch(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks)
    : MosaicAlgorithmBase(imageDatas),
      _ImageDatas(imageDatas),
      _CloudMasks(cloudMasks),
      _MosaicCloudMask() {}

void AdaptiveIsophotePatch::Execute() {
    SuperDebug::Info("Mosaic Image Size: {} x {}", AlgorithmResult.Height(), AlgorithmResult.Width());

    if (!(!_CloudMasks.empty() && _CloudMasks.size() == _ImageDatas.size())) {
        SuperDebug::Error("AdaptiveIsophotePatch requires cloud masks with the same count as input images.");
        SuperDebug::Error("Current behavior fallback: perform plain geographic mosaic without cloud-aware reconstruction.");
        for (const auto &imgData : _ImageDatas) {
            _PasteImageToMosaicResult(imgData);
        }
        SuperDebug::Info("Mosaic Completed.");
        return;
    }

    RSPIP::Util::SortImagesByLongitude(_ImageDatas);
    RSPIP::Util::SortImagesByLongitude(_CloudMasks);
    _InitCloudMasks();

    _MosaicWithoutCloud();
    _BuildMosaicCloudMask();

    _IsophoteReconstruct();

    SuperDebug::Info("Mosaic Completed.");
}

void AdaptiveIsophotePatch::_InitCloudMasks() {
    for (size_t index = 0; index < _CloudMasks.size(); ++index) {
        _CloudMasks[index].SetSourceImage(_ImageDatas[index]);
        _CloudMasks[index].Init();
    }
}

void AdaptiveIsophotePatch::_MosaicWithoutCloud() {
    cv::Mat mosaicMask = cv::Mat::zeros(AlgorithmResult.Height(), AlgorithmResult.Width(), CV_8UC1);
    _MosaicCloudMask = CloudMask(mosaicMask);
    _MosaicCloudMask.SetSourceImage(AlgorithmResult);

    for (size_t index = 0; index < _ImageDatas.size(); ++index) {
        _PasteClearPixelsToMosaicResult(_ImageDatas[index], _CloudMasks[index]);
    }
}

void AdaptiveIsophotePatch::_PasteClearPixelsToMosaicResult(const GeoImage &imageData, const CloudMask &cloudMask) {
    int columns = imageData.Width();
    int rows = imageData.Height();

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

            // 任意一景被标记为云，则在镶嵌云掩膜中锁定该位置，并清空结果像素留给阶段二重建
            if (!cloudMask.ImageData.empty() && cloudMask.GetPixelValue<unsigned char>(row, col) != cloudMask.NonData[0]) {
                _MosaicCloudMask.SetPixelValue<unsigned char>(mosaicRow, mosaicColumn, 255);
                AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, AlgorithmResult.NonData);
                continue;
            }

            // 云并集中的位置不允许阶段一填充，避免后续影像“洗掉”云信息
            if (_MosaicCloudMask.GetPixelValue<unsigned char>(mosaicRow, mosaicColumn) != _MosaicCloudMask.NonData[0]) {
                continue;
            }

            if (AlgorithmResult.GetPixelValue<cv::Vec3b>(mosaicRow, mosaicColumn) != AlgorithmResult.NonData) {
                continue;
            }

            AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
        }
    }
}

void AdaptiveIsophotePatch::_BuildMosaicCloudMask() {
    _MosaicCloudMask.SetSourceImage(AlgorithmResult);
    _MosaicCloudMask.Init();
}

void AdaptiveIsophotePatch::_IsophoteReconstruct() {
    if (_MosaicCloudMask.CloudGroups.empty()) {
        SuperDebug::Info("No cloud region remains after stage1, skip Isophote reconstruction.");
        return;
    }

    GeoImage referMosaic = AlgorithmResult;
    referMosaic.NonData = AlgorithmResult.NonData;

    _FillReferMosaicWithCloudGroups(referMosaic);

    // DebugTest
    // auto GeoSaveImagePath = "E:/RSPIP/Resource/Temp/";
    // auto GeoSaveImageName = "ReferMosaic.tif";
    // IO::SaveImage(referMosaic, GeoSaveImagePath, GeoSaveImageName);

    ReconstructAlgorithm::IsophoteConstrain reconstructAlgorithm(AlgorithmResult, referMosaic, _MosaicCloudMask);
    reconstructAlgorithm.Execute();
    AlgorithmResult.ImageData = reconstructAlgorithm.AlgorithmResult.ImageData.clone();
}

void AdaptiveIsophotePatch::_FillReferMosaicWithCloudGroups(GeoImage &referMosaic) {
    for (const auto &cloudGroup : _MosaicCloudMask.CloudGroups) {
        _FillReferMosaicCloudGroup(referMosaic, cloudGroup);
    }
}

void AdaptiveIsophotePatch::_FillReferMosaicCloudGroup(GeoImage &referMosaic, const CloudGroup &cloudGroup) {
    for (const auto &[_, rowCloudPixels] : cloudGroup.CloudPixelMap) {
        for (size_t cloudPixelIndex = 0; cloudPixelIndex < rowCloudPixels.size(); ++cloudPixelIndex) {
            const auto &cloudPixel = rowCloudPixels[cloudPixelIndex];
            auto [mosaicRow, mosaicColumn] = std::pair{cloudPixel.Row, cloudPixel.Column};

            if (referMosaic.GetPixelValue<cv::Vec3b>(mosaicRow, mosaicColumn) != referMosaic.NonData) {
                continue;
            }

            const auto bestPatch = _SelectBestPatchForRefer(rowCloudPixels, cloudPixelIndex);
            if (bestPatch.empty()) {
                continue;
            }

            for (const auto &patchPixel : bestPatch) {
                auto [patchMosaicRow, patchMosaicColumn] = AlgorithmResult.LatLonToRC(patchPixel.Latitude, patchPixel.Longitude);
                if (patchMosaicRow == -1 || patchMosaicColumn == -1) {
                    continue;
                }
                referMosaic.SetPixelValue(patchMosaicRow, patchMosaicColumn, patchPixel.Value);
            }

            cloudPixelIndex += bestPatch.size() - 1;
        }
    }
}

std::vector<GeoPixel<cv::Vec3b>> AdaptiveIsophotePatch::_SelectBestPatchForRefer(const std::vector<CloudPixel<unsigned char>> &rowCloudPixels, size_t startCloudPixelIndex) {
    double bestPearsonCorrelation = -2.0;
    size_t bestPatchLength = 0;
    bool foundPatchWithCorrelation = false;
    std::vector<GeoPixel<cv::Vec3b>> bestPatch = {};

    for (size_t otherImageIndex = 0; otherImageIndex < _ImageDatas.size(); ++otherImageIndex) {
        const auto &otherImage = _ImageDatas[otherImageIndex];
        const auto &otherMask = _CloudMasks[otherImageIndex];

        std::vector<double> mosaicResultPreviousLine = {};
        std::vector<double> otherImagePreviousLine = {};
        std::vector<GeoPixel<cv::Vec3b>> currentPatch = {};

        for (size_t cloudPixelIndex = startCloudPixelIndex; cloudPixelIndex < rowCloudPixels.size(); ++cloudPixelIndex) {
            const auto &nextCloudPixel = rowCloudPixels[cloudPixelIndex];
            if (!otherImage.IsContain(nextCloudPixel.Latitude, nextCloudPixel.Longitude)) {
                break;
            }

            auto [otherImagePixelRow, otherImagePixelCol] = otherImage.LatLonToRC(nextCloudPixel.Latitude, nextCloudPixel.Longitude);
            if (otherImage.IsOutOfBounds(otherImagePixelRow, otherImagePixelCol)) {
                break;
            }

            if (!otherMask.ImageData.empty() && otherMask.GetPixelValue<unsigned char>(otherImagePixelRow, otherImagePixelCol) != otherMask.NonData[0]) {
                break;
            }

            auto currentPatchPixelValue = otherImage.GetPixelValue<cv::Vec3b>(otherImagePixelRow, otherImagePixelCol);
            if (currentPatchPixelValue == otherImage.NonData) {
                break;
            }

            currentPatch.emplace_back(currentPatchPixelValue, otherImagePixelRow, otherImagePixelCol, nextCloudPixel.Latitude, nextCloudPixel.Longitude);

            if (otherImagePixelRow == 0 || nextCloudPixel.Row == 0) {
                continue;
            }

            auto otherPreviousLineValue = otherImage.GetPixelValue<cv::Vec3b>(otherImagePixelRow - 1, otherImagePixelCol);
            if (otherPreviousLineValue == otherImage.NonData) {
                continue;
            }

            if (_MosaicCloudMask.GetPixelValue<unsigned char>(nextCloudPixel.Row - 1, nextCloudPixel.Column) != _MosaicCloudMask.NonData[0]) {
                continue;
            }

            auto mosaicPreviousLineValue = AlgorithmResult.GetPixelValue<cv::Vec3b>(nextCloudPixel.Row - 1, nextCloudPixel.Column);
            if (mosaicPreviousLineValue == AlgorithmResult.NonData) {
                continue;
            }

            otherImagePreviousLine.emplace_back(Util::BGRToGray(otherPreviousLineValue));
            mosaicResultPreviousLine.emplace_back(Util::BGRToGray(mosaicPreviousLineValue));
        }

        if (currentPatch.empty()) {
            continue;
        }

        bool hasCorrelation = mosaicResultPreviousLine.size() >= 2 && otherImagePreviousLine.size() >= 2;
        double pearsonCorrelation = hasCorrelation ? Math::PearsonCorrelation(mosaicResultPreviousLine, otherImagePreviousLine) : -2.0;

        if (hasCorrelation) {
            if (!foundPatchWithCorrelation || pearsonCorrelation > bestPearsonCorrelation ||
                (pearsonCorrelation == bestPearsonCorrelation && currentPatch.size() > bestPatchLength)) {
                bestPatch = std::move(currentPatch);
                bestPearsonCorrelation = pearsonCorrelation;
                bestPatchLength = bestPatch.size();
                foundPatchWithCorrelation = true;
            }
            continue;
        }

        if (!foundPatchWithCorrelation && currentPatch.size() > bestPatchLength) {
            bestPatch = std::move(currentPatch);
            bestPatchLength = bestPatch.size();
        }
    }

    return bestPatch;
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm
