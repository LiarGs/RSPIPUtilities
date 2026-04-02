#include "Algorithm/Reconstruct/ColorBalanceReconstruct.h"
#include "Algorithm/Detail/AlgorithmValidation.h"
#include "Algorithm/Detail/MaskPolicies.h"
#include "Algorithm/ColorBalance/MatchStatistic.h"
#include "Basic/RegionExtraction.h"

namespace RSPIP::Algorithm::ReconstructAlgorithm {

ColorBalanceReconstruct::ColorBalanceReconstruct(Image reconstructImage, Image referImage, Image maskImage)
    : _ReconstructImage(std::move(reconstructImage)), _ReferImage(std::move(referImage)), _Mask(std::move(maskImage)) {}

void ColorBalanceReconstruct::Execute() {
    ColorBalanceAlgorithm::MatchStatistics matchAlgo(_ReferImage, _ReconstructImage, _Mask);
    matchAlgo.Execute();
    if (matchAlgo.AlgorithmResult.ImageData.empty()) {
        Detail::ResetImageResult(AlgorithmResult);
        return;
    }

    AlgorithmResult = _ReconstructImage;
    const auto selectionMask = BuildSelectionMask(_Mask, Detail::DefaultBinaryMaskPolicy());
    if (selectionMask.empty()) {
        Detail::ResetImageResult(AlgorithmResult);
        return;
    }

    matchAlgo.AlgorithmResult.ImageData.copyTo(AlgorithmResult.ImageData, selectionMask);
}

} // namespace RSPIP::Algorithm::ReconstructAlgorithm
