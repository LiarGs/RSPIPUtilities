#pragma once
#include "Algorithm/Preprocess/PreprocessAlgorithmBase.h"

namespace RSPIP::Algorithm::PreprocessAlgorithm {

class GeoCoordinateAlign : public PreprocessAlgorithmBase {
  public:
    explicit GeoCoordinateAlign(std::vector<Image> imageDatas);
    GeoCoordinateAlign(std::vector<Image> imageDatas, std::vector<Image> maskImages);

    void Execute() override;

  private:
    struct LocalGridWindow {
        std::vector<double> GeoTransform;
        int Rows = 0;
        int Columns = 0;
    };

    struct WarpPreparation {
        LocalGridWindow LocalGrid;
        cv::Mat MapX;
        cv::Mat MapY;
    };

  private:
    bool _BuildUnifiedGrid();
    LocalGridWindow _BuildLocalGridWindow(const Image &imageData) const;
    bool _CanReuseWarpPreparation(const Image &imageData, const Image &otherImage) const;
    WarpPreparation _PrepareWarp(const Image &imageData, const LocalGridWindow &localGrid) const;
    Image _WarpImage(const Image &imageData, const WarpPreparation &warpPreparation) const;
    Image _WarpMaskImage(const Image &maskImage, const WarpPreparation &warpPreparation) const;
};

} // namespace RSPIP::Algorithm::PreprocessAlgorithm
