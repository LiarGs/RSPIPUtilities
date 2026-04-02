#include "Algorithm/Reconstruct/Simple.h"
#include "Algorithm/Detail/AlgorithmValidation.h"
#include "Algorithm/Detail/MaskPolicies.h"
#include "Basic/RegionExtraction.h"

namespace RSPIP::Algorithm::ReconstructAlgorithm {

Simple::Simple(Image reconstructImage, Image referImage, Image maskImage)
    : _ReconstructImage(std::move(reconstructImage)), _ReferImage(std::move(referImage)), _Mask(std::move(maskImage)) {
    AlgorithmResult = _ReconstructImage;
}

void Simple::Execute() {
    const auto selectionMask = BuildSelectionMask(_Mask, Detail::DefaultBinaryMaskPolicy());
    if (selectionMask.empty()) {
        Detail::ResetImageResult(AlgorithmResult);
        return;
    }

    _ReferImage.ImageData.copyTo(AlgorithmResult.ImageData, selectionMask);
}

} // namespace RSPIP::Algorithm::ReconstructAlgorithm
