#include "Algorithm/ImageReconstruct/ColorBalanceReconstruct.h"
#include "Algorithm/ColorBalance/MatchStatistic.h"

namespace RSPIP::Algorithm::ReconstructAlgorithm {

ColorBalanceReconstruct::ColorBalanceReconstruct(const Image &reconstructImage, const Image &referImage, const CloudMask &maskImage)
    : _ReconstructImage(reconstructImage), _ReferImage(referImage), _Mask(maskImage) {}

void ColorBalanceReconstruct::Execute() {
    ColorBalanceAlgorithm::MatchStatistics matchAlgo(_ReferImage, _ReconstructImage, _Mask);
    matchAlgo.Execute();
    AlgorithmResult = std::move(matchAlgo.AlgorithmResult);
    AlgorithmResult.ImageData.copyTo(AlgorithmResult.ImageData, _Mask.ImageData);
}

} // namespace RSPIP::Algorithm::ReconstructAlgorithm