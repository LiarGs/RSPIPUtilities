#include "Algorithm/Mosaic/AdaptivePatch.h"

namespace RSPIP::Algorithm::MosaicAlgorithm {

AdaptivePatch::AdaptivePatch(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks)
    : AdaptiveStripMosaicBase(imageDatas, cloudMasks) {}

const char *AdaptivePatch::_AlgorithmName() const {
    return "AdaptivePatch";
}

AdaptivePatch::PatchApplyResult AdaptivePatch::_ApplyCandidatePatch(const CandidatePatch &candidate, ExpandDirection direction, int boundaryRow, int boundaryColumn) {
    if (!candidate.IsValid || candidate.InputIndex >= _Inputs.size() || candidate.Length <= 0) {
        return PatchApplyResult::Failed;
    }

    _CopyPatchToResult(_Inputs[candidate.InputIndex], direction, boundaryRow, boundaryColumn, candidate.Length, candidate.InputIndex);
    return PatchApplyResult::PrimaryApplied;
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm
