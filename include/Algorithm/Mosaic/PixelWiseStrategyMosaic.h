#pragma once
#include "Algorithm/Mosaic/MosaicAlgorithmBase.h"

namespace RSPIP::Algorithm::MosaicAlgorithm {

enum class PixelWiseOverlapStrategy {
    LastWriteWins,
    HighlightOverlapRed,
    MeanOfValidPixels,
    MedianOfValidPixels
};

class PixelWiseStrategyMosaic : public MosaicAlgorithmBase {
  public:
    explicit PixelWiseStrategyMosaic(std::vector<Image> imageDatas);
    PixelWiseStrategyMosaic(std::vector<Image> imageDatas, std::vector<Image> maskImages);

    void SetOverlapStrategy(PixelWiseOverlapStrategy strategy);
    void Execute() override;

  private:
    bool _ValidateExecuteInputs() const;
    bool _ValidateAggregateStrategyInputs() const;
    bool _BuildAggregateSelectionMasks(std::vector<cv::Mat> &selectionMasks) const;
    bool _TryProjectPixel(const Image &imageData, int row, int column, const cv::Mat *selectionMask,
                          int &mosaicRow, int &mosaicColumn, cv::Vec3b &pixelValue) const;
    void _ExecuteLastWriteWins();
    void _ExecuteHighlightOverlapRed();
    void _ExecuteMeanOfValidPixels(const std::vector<cv::Mat> &selectionMasks);
    void _ExecuteMedianOfValidPixels(const std::vector<cv::Mat> &selectionMasks);
    void _AccumulateImageToBuffers(const Image &imageData, const cv::Mat *selectionMask, cv::Mat &sumBuffer, cv::Mat &countBuffer) const;
    void _ResolveMeanResult(const cv::Mat &sumBuffer, const cv::Mat &countBuffer);
    void _AccumulateImageToSampleBuckets(const Image &imageData, const cv::Mat *selectionMask, std::vector<std::vector<cv::Vec3b>> &sampleBuckets) const;
    void _ResolveMedianResult(const std::vector<std::vector<cv::Vec3b>> &sampleBuckets);

  private:
    std::vector<Image> _MaskImages;
    PixelWiseOverlapStrategy _OverlapStrategy = PixelWiseOverlapStrategy::LastWriteWins;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm
