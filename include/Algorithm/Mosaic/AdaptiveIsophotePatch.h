#pragma once
#include "Algorithm/Mosaic/Detail/AdaptiveStripMosaicBase.h"

namespace RSPIP::Algorithm::MosaicAlgorithm {

class AdaptiveIsophotePatch : public Detail::AdaptiveStripMosaicBase {
  public:
    AdaptiveIsophotePatch(std::vector<Image> imageDatas, std::vector<Image> maskImages);

    using Detail::AdaptiveStripMosaicBase::SetStripWidth;

    void SetMaxIterations(int newMaxIterations) {
        _MaxIterations = newMaxIterations;
    }

    void SetEpsilon(double newEpsilon) {
        _Epsilon = newEpsilon;
    }

  protected:
    using Detail::AdaptiveStripMosaicBase::CandidatePatch;
    using Detail::AdaptiveStripMosaicBase::ExpandDirection;
    using Detail::AdaptiveStripMosaicBase::InputBundle;
    using Detail::AdaptiveStripMosaicBase::PatchApplyResult;
    using Detail::AdaptiveStripMosaicBase::PatchPixel;

    const char *_AlgorithmName() const override;
    bool _ShouldSortInputsByLongitude() const override {
        return true;
    }
    bool _NeedsGlobalValidStatistics() const override {
        return true;
    }
    PatchApplyResult _ApplyCandidatePatch(const CandidatePatch &candidate, ExpandDirection direction, int boundaryRow, int boundaryColumn) override;

  private:
    bool _ReconstructPatchLocally(const std::vector<PatchPixel> &patchPixels, size_t ownerInputIndex);
    Image _CreateLocalImage(const cv::Rect &window) const;

  private:
    int _MaxIterations = 10000;
    double _Epsilon = 1;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm
