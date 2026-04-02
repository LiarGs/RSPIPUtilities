#pragma once
#include "Algorithm/Mosaic/Detail/AdaptiveStripMosaicBase.h"

namespace RSPIP::Algorithm::MosaicAlgorithm {

class AdaptiveColorBalancePatch : public Detail::AdaptiveStripMosaicBase {
  public:
    AdaptiveColorBalancePatch(std::vector<Image> imageDatas, std::vector<Image> maskImages);

    using Detail::AdaptiveStripMosaicBase::SetStripWidth;

  protected:
    using Detail::AdaptiveStripMosaicBase::CandidatePatch;
    using Detail::AdaptiveStripMosaicBase::ExpandDirection;
    using Detail::AdaptiveStripMosaicBase::PatchApplyResult;

    const char *_AlgorithmName() const override;
    bool _NeedsGlobalValidStatistics() const override {
        return true;
    }
    PatchApplyResult _ApplyCandidatePatch(const CandidatePatch &candidate, ExpandDirection direction, int boundaryRow, int boundaryColumn) override;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm
