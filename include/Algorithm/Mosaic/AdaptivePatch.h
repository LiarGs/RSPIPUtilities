#pragma once
#include "Algorithm/Mosaic/Detail/AdaptiveStripMosaicBase.h"

namespace RSPIP::Algorithm::MosaicAlgorithm {

class AdaptivePatch : public Detail::AdaptiveStripMosaicBase {
  public:
    AdaptivePatch(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks);

    using Detail::AdaptiveStripMosaicBase::SetStripWidth;

  protected:
    using Detail::AdaptiveStripMosaicBase::CandidatePatch;
    using Detail::AdaptiveStripMosaicBase::ExpandDirection;
    using Detail::AdaptiveStripMosaicBase::InputBundle;
    using Detail::AdaptiveStripMosaicBase::PatchApplyResult;

    const char *_AlgorithmName() const override;
    PatchApplyResult _ApplyCandidatePatch(const CandidatePatch &candidate, ExpandDirection direction, int boundaryRow, int boundaryColumn) override;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm
