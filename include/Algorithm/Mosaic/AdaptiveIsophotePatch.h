#pragma once
#include "Algorithm/Mosaic/MosaicAlgorithmBase.h"
#include "Basic/CloudMask.h"
#include "Basic/GeoPixel.h"

namespace RSPIP::Algorithm::MosaicAlgorithm {

class AdaptiveIsophotePatch : public MosaicAlgorithmBase {
  public:
    AdaptiveIsophotePatch(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks);

    void Execute() override;

  private:
    void _InitCloudMasks();
    void _MosaicWithoutCloud();
    void _PasteClearPixelsToMosaicResult(const GeoImage &imageData, const CloudMask &cloudMask);
    void _BuildMosaicCloudMask();
    void _IsophoteReconstruct();
    void _FillReferMosaicWithCloudGroups(GeoImage &referMosaic);
    void _FillReferMosaicCloudGroup(GeoImage &referMosaic, const CloudGroup &cloudGroup);
    std::vector<GeoPixel<cv::Vec3b>> _SelectBestPatchForRefer(const std::vector<CloudPixel<unsigned char>> &rowCloudPixels, size_t startCloudPixelIndex);

  private:
    std::vector<GeoImage> _ImageDatas;
    std::vector<CloudMask> _CloudMasks;
    CloudMask _MosaicCloudMask;
};
} // namespace RSPIP::Algorithm::MosaicAlgorithm
