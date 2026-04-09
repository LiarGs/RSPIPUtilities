#pragma once
#include "Algorithm/Mosaic/MosaicAlgorithmBase.h"
#include <string>

namespace RSPIP::Algorithm::MosaicAlgorithm {

class VoronoiPartitionMosaic : public MosaicAlgorithmBase {
  public:
    explicit VoronoiPartitionMosaic(std::vector<Image> imageDatas);
    VoronoiPartitionMosaic(std::vector<Image> imageDatas, std::vector<Image> maskImages);

    void Execute() override;

  private:
    struct InputBundle {
        Image SourceImage;
        Image Mask;
        cv::Mat SelectionMask;
        size_t OriginalIndex = 0;
        size_t ValidProjectedPixelCount = 0;
        double SeedRow = 0.0;
        double SeedColumn = 0.0;
        bool IsEnabled = true;
        std::string DisabledReason = {};
    };

  private:
    bool _ValidateExecuteInputs() const;
    bool _PrepareInputs();
    bool _BuildSelectionMask(InputBundle &input) const;
    bool _IsSelectedByMask(const cv::Mat &selectionMask, int row, int column) const;
    bool _IsSourcePixelValid(const InputBundle &input, int row, int column) const;
    bool _TryProjectSourcePixelToMosaic(const InputBundle &input, int sourceRow, int sourceColumn, int &mosaicRow, int &mosaicColumn) const;
    bool _TryResolveSourcePixelForWorldLocation(const InputBundle &input, double latitude, double longitude, cv::Vec3b &pixelValue) const;
    double _ComputeSeedDistanceSquared(const InputBundle &input, int mosaicRow, int mosaicColumn) const;

  private:
    std::vector<InputBundle> _Inputs;
    bool _HasMaskInputs = false;
    size_t _ProvidedMaskCount = 0;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm
