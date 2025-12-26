/// Inspired by paper
/// Missing information reconstruction integrating isophote constraint and color-structure control for remote sensing data
/// by YU Xiao Yu

#pragma once
#include "Algorithm/ImageReconstruct/ReconstructAlgorithmBase.h"
#include "Basic/CloudMask.h"
#include "Math/LinearSystem/SparseMatrix.h"

namespace RSPIP::Algorithm::ReconstructAlgorithm {

class IsophoteConstrain : public ReconstructAlgorithmBase {
  public:
    IsophoteConstrain(const Image &reconstructImage, const Image &referImage, const CloudMask &maskImage);
    void Execute() override;

  private:
    void _ReconstructCloudGroup(const CloudGroup &cloudGroup);
    std::tuple<Math::LinearSystem::SparseMatrix, cv::Mat, cv::Mat> _BuildLinearSystem(const CloudGroup &cloudGroup, int channelNum);
    void _BuildLinearSystemRow(const CloudPixel<unsigned char> &pixel, int channelNum);

  private:
    const Image &_ReconstructImage;
    Image _ReferImage;
    CloudMask _Mask;
};
} // namespace RSPIP::Algorithm::ReconstructAlgorithm