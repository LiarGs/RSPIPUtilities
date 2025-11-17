// Create By GuoShuai
// Idea From YuXiaoYu
// A flexible multi-temporal orthoimage mosaicking method based on dynamic variable patches

#pragma once
#include "Basic/CloudMask.hpp"
#include "Math/Statistics.hpp"
#include "MosaicAlgorithmBase.hpp"
#include "Util/General.hpp"
#include "Util/SortImage.hpp"

namespace RSPIP::Algorithm::MosaicAlgorithm {

struct DynamicPatchMosaicParams : BasicMosaicParam {

  public:
    DynamicPatchMosaicParams(const std::vector<std::shared_ptr<GeoImage>> &imageDatas, const std::vector<std::shared_ptr<CloudMask>> &cloudMasks)
        : BasicMosaicParam(imageDatas), CloudMasks(cloudMasks) {}

  public:
    std::vector<std::shared_ptr<CloudMask>> CloudMasks;
};

class DynamicPatchMosaic : public MosaicAlgorithmBase {
  public:
    void Execute(std::shared_ptr<AlgorithmParamBase> params) override {
        if (auto dynamicPatchMosaicParams = std::dynamic_pointer_cast<DynamicPatchMosaicParams>(params)) {
            _InitMosaicParameters(dynamicPatchMosaicParams);
            _ImageDatas = dynamicPatchMosaicParams->ImageDatas;
            _CloudMasks = dynamicPatchMosaicParams->CloudMasks;

            RSPIP::Util::SortImagesByLongitude(_ImageDatas);
            RSPIP::Util::SortImagesByLongitude(_CloudMasks);
            RSPIP::Util::MatchMaskWithSource(_CloudMasks, _ImageDatas);

            _MosaicImages();
        }
    }

  private:
    void _MosaicImages() {
        Info("Mosaic Image Size: {} x {}", MosaicResult->Height(), MosaicResult->Width());
        for (size_t currentImageIndex = 0; currentImageIndex < _ImageDatas.size(); ++currentImageIndex) {
            _CurrentProcessImageIndex = currentImageIndex;
            _PasteImageToMosaicResult(_ImageDatas[_CurrentProcessImageIndex]);
            _ProcessWithClouds();
        }

        Info("Mosaic Completed.");
    }

    void _PasteImageToMosaicResult(const std::shared_ptr<GeoImage> &imageData) override {
        auto columns = imageData->Width();
        auto rows = imageData->Height();

        Info("Pasting Image: {} Size: {} x {}", imageData->ImageName, rows, columns);

        for (size_t row = 0; row < rows; ++row) {
            for (size_t col = 0; col < columns; ++col) {

                auto pixelValue = imageData->GetPixelValue<cv::Vec3b>(row, col);
                if (pixelValue == imageData->NonData) {
                    continue;
                }

                auto [latitude, longitude] = imageData->GetLatLon(row, col);
                auto [mosaicRow, mosaicColumn] = MosaicResult->LatLonToRC(latitude, longitude);
                if (mosaicRow == -1 || mosaicColumn == -1) {
                    continue;
                }

                if (MosaicResult->GetPixelValue<cv::Vec3b>(mosaicRow, mosaicColumn) != MosaicResult->NonData) {
                    continue;
                }

                MosaicResult->SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
            }

            std::cout << "\rPasting Image: " << static_cast<int>((row + 1) * 100.0 / rows) << "%" << std::flush;
        }
        std::cout << std::endl;
    }

    void _ProcessWithClouds() {
        const auto &cloudMask = _CloudMasks[_CurrentProcessImageIndex];
        for (size_t cloudGroupIndex = 0; cloudGroupIndex < cloudMask->CloudGroups.size(); ++cloudGroupIndex) {
            Info("Processing Cloud Group: {}/{}", cloudGroupIndex + 1, cloudMask->CloudGroups.size());
            _ProcessWithCloud(cloudMask->CloudGroups[cloudGroupIndex]);
        }
    }

    void _ProcessWithCloud(const CloudGroup &cloudGroup) {
        for (const auto &pair : cloudGroup.CloudPixelMap) {
            auto &rowCloudPixels = pair.second;
            for (size_t cloudPixelIndex = 0; cloudPixelIndex < rowCloudPixels.size(); ++cloudPixelIndex) {
                const auto &cloudPixel = rowCloudPixels[cloudPixelIndex];

                auto [latitude, longitude] = std::pair{cloudPixel.Latitude, cloudPixel.Longitude};
                auto [mosaicRow, mosaicColumn] = MosaicResult->LatLonToRC(latitude, longitude);

                if (mosaicRow == -1 || mosaicColumn == -1) {
                    continue;
                }

                if (MosaicResult->GetPixelValue<cv::Vec3b>(mosaicRow, mosaicColumn)[0] != cloudPixel.Value) {
                    // 此处云已被填充
                    continue;
                }

                auto bestPatch = _OptimalPatchSelection(cloudPixelIndex, rowCloudPixels);

                if (bestPatch.empty()) {
                    continue;
                }

                for (const auto &pixel : bestPatch) {
                    auto [mosaicResultPixelRow, mosaicResultPixelCol] = MosaicResult->LatLonToRC(pixel.Latitude, pixel.Longitude);
                    MosaicResult->SetPixelValue(mosaicResultPixelRow, mosaicResultPixelCol, pixel.Value);
                }
            }
        }
    }

    std::vector<GeoPixel<cv::Vec3b>> _OptimalPatchSelection(size_t currentCloudPixelIndex, const std::vector<GeoPixel<uchar>> &rowCloudPixels) {
        double bestPearsonCorrelation = -1.0;
        std::vector<GeoPixel<cv::Vec3b>> bestPatch;

        for (size_t otherImageIndex = 0; otherImageIndex < _ImageDatas.size(); ++otherImageIndex) {
            if (otherImageIndex == _CurrentProcessImageIndex) {
                continue;
            }
            const auto &otherImage = _ImageDatas[otherImageIndex];

            std::vector<double> mosaicResultPreviousLine = {};
            std::vector<double> otherImagePreviousLine = {};
            std::vector<GeoPixel<cv::Vec3b>> currentPatch = {};

            // 收集上一行对应的数据
            for (size_t cloudPixelIndex = currentCloudPixelIndex; cloudPixelIndex < rowCloudPixels.size(); ++cloudPixelIndex) {

                const auto &nextCloudPixel = rowCloudPixels[cloudPixelIndex];
                if (!otherImage->IsContain(nextCloudPixel.Latitude, nextCloudPixel.Longitude)) {
                    break;
                }

                auto [otherImagePixelRow, otherImagePixelCol] = otherImage->LatLonToRC(nextCloudPixel.Latitude, nextCloudPixel.Longitude);
                if (otherImagePixelRow == 0) {
                    break;
                }

                auto pixelValue = otherImage->GetPixelValue<cv::Vec3b>(otherImagePixelRow - 1, otherImagePixelCol);
                if (pixelValue == otherImage->NonData) {
                    break;
                }

                otherImagePreviousLine.emplace_back(Util::BGRToGray(pixelValue));

                auto [mosaicResultPixelRow, mosaicResultPixelCol] = MosaicResult->LatLonToRC(nextCloudPixel.Latitude, nextCloudPixel.Longitude);
                mosaicResultPreviousLine.emplace_back(Util::BGRToGray(MosaicResult->GetPixelValue<cv::Vec3b>(mosaicResultPixelRow - 1, mosaicResultPixelCol)));

                auto patchPixel = GeoPixel<cv::Vec3b>(otherImage->GetPixelValue<cv::Vec3b>(otherImagePixelRow, otherImagePixelCol), otherImagePixelRow, otherImagePixelCol, nextCloudPixel.Latitude, nextCloudPixel.Longitude);
                currentPatch.emplace_back(patchPixel);
            }

            if (mosaicResultPreviousLine.empty() || otherImagePreviousLine.empty()) {
                continue;
            }
            auto pearsonCorrelation = Math::PearsonCorrelation(mosaicResultPreviousLine, otherImagePreviousLine);

            if (pearsonCorrelation > bestPearsonCorrelation) {
                bestPatch = currentPatch;
                bestPearsonCorrelation = pearsonCorrelation;
            }
        }

        return bestPatch;
    }

  private:
    std::vector<std::shared_ptr<GeoImage>> _ImageDatas;
    std::vector<std::shared_ptr<CloudMask>> _CloudMasks;

  private:
    size_t _CurrentProcessImageIndex;
};
} // namespace RSPIP::Algorithm::MosaicAlgorithm