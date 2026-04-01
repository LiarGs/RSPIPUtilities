#include "Algorithm/Mosaic/AdaptiveColorBalancePatch.h"
#include "Util/General.h"

namespace RSPIP::Algorithm::MosaicAlgorithm {

AdaptiveColorBalancePatch::AdaptiveColorBalancePatch(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks)
    : AdaptiveStripMosaicBase(imageDatas, cloudMasks) {}

const char *AdaptiveColorBalancePatch::_AlgorithmName() const {
    return "AdaptiveColorBalancePatch";
}

AdaptiveColorBalancePatch::PatchApplyResult AdaptiveColorBalancePatch::_ApplyCandidatePatch(const CandidatePatch &candidate, ExpandDirection direction, int boundaryRow, int boundaryColumn) {
    if (!candidate.IsValid || candidate.InputIndex >= _Inputs.size() || candidate.Length <= 0) {
        return PatchApplyResult::Failed;
    }

    std::vector<PatchPixel> patchPixels = {};
    bool hasCompleteBoundary = false;
    if (!_BuildPatchPixels(_Inputs[candidate.InputIndex], direction, boundaryRow, boundaryColumn, candidate.Length, patchPixels, hasCompleteBoundary) || patchPixels.empty()) {
        return PatchApplyResult::Failed;
    }

    if (hasCompleteBoundary) {
        std::vector<cv::Vec3b> balancedValues = {};
        if (_ComputeBalancedValues(patchPixels, balancedValues) && balancedValues.size() == patchPixels.size()) {
            for (size_t index = 0; index < patchPixels.size(); ++index) {
                const auto &balancedValue = balancedValues[index];
                _SetFilledPixel(
                    patchPixels[index].MosaicRow,
                    patchPixels[index].MosaicColumn,
                    balancedValue,
                    static_cast<unsigned char>(Util::BGRToGray(balancedValue)),
                    static_cast<int>(candidate.InputIndex));
            }
            return PatchApplyResult::PrimaryApplied;
        }
    }

    _PastePatchDirectly(patchPixels, candidate.InputIndex);
    return PatchApplyResult::FallbackApplied;
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm
