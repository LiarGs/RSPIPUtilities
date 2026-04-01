#pragma once
#include "Algorithm/Mosaic/Detail/AdaptiveStripMosaicBase.h"

namespace RSPIP::Algorithm::MosaicAlgorithm {

class AdaptiveIsophotePatch : public Detail::AdaptiveStripMosaicBase {
  public:
    AdaptiveIsophotePatch(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks);

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
    void _OnInputBundlePrepared(InputBundle &input) override;
    PatchApplyResult _ApplyCandidatePatch(const CandidatePatch &candidate, ExpandDirection direction, int boundaryRow, int boundaryColumn) override;

  private:
    bool _ReconstructPatchLocally(const std::vector<PatchPixel> &patchPixels, size_t ownerInputIndex);
    GeoImage _CreateLocalGeoImage(const cv::Rect &window) const;

  private:
    int _MaxIterations = 10000;
    double _Epsilon = 1;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm
