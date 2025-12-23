#pragma once
#include "Algorithm/Mosaic/MosaicAlgorithmBase.h"
#include "Basic/CloudMask.h"
#include <vector>

namespace RSPIP::Algorithm::MosaicAlgorithm {

class DynamicPatch : public MosaicAlgorithmBase {
  public:
    DynamicPatch(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks);

    void Execute() override;

  private:
    void _PasteImageToMosaicResult(const GeoImage &imageData) override;
    void _ProcessWithClouds();
    void _ProcessWithCloudGroup(const CloudGroup &cloudGroup);
    std::vector<GeoPixel<cv::Vec3b>> _OptimalPatchSelection(const std::vector<GeoPixel<uchar>> &rowCloudPixels, size_t currentCloudPixelIndex);

  private:
    /// @brief 这里用拷贝是因为要对数据排序
    std::vector<GeoImage> _ImageDatas;
    std::vector<CloudMask> _CloudMasks;

    size_t _CurrentProcessImageIndex;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm