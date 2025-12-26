#include "Algorithm/ImageReconstruct/Simple.h"

namespace RSPIP::Algorithm::ReconstructAlgorithm {

Simple::Simple(const Image &reconstructImage, const Image &referImage, const CloudMask &maskImage)
    : _ReconstructImage(reconstructImage), _ReferImage(referImage), _Mask(maskImage) { AlgorithmResult = reconstructImage; }

void Simple::Execute() {
    _ReferImage.ImageData.copyTo(AlgorithmResult.ImageData, _Mask.ImageData);
}

} // namespace RSPIP::Algorithm::ReconstructAlgorithm
