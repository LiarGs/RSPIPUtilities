#pragma once
#include "Algorithm/Preprocess/PreprocessAlgorithmBase.h"

namespace RSPIP::Algorithm::PreprocessAlgorithm {

class GeoCoordinateAlign : public PreprocessAlgorithmBase {
  public:
    explicit GeoCoordinateAlign(const std::vector<GeoImage> &imageDatas);
    GeoCoordinateAlign(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks);

    void Execute() override;

  private:
    struct LocalGridWindow {
        std::vector<double> GeoTransform;
        int Rows = 0;
        int Columns = 0;
    };

  private:
    void _BuildUnifiedGrid();
    LocalGridWindow _BuildLocalGridWindow(const GeoImage &imageData) const;
    GeoImage _WarpGeoImage(const GeoImage &imageData, const LocalGridWindow &localGrid) const;
    CloudMask _WarpCloudMask(const CloudMask &cloudMask, const LocalGridWindow &localGrid) const;
};

} // namespace RSPIP::Algorithm::PreprocessAlgorithm
