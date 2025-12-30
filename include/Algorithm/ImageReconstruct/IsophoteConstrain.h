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
    void _BuildLinearSystem(const CloudGroup &cloudGroup, int channelNum);
    void _BuildLinearSystemRow(const CloudGroup &cloudGroup, const CloudPixel<unsigned char> &pixel, int channelNum);

  private:
    const Image &_ReconstructImage;
    Image _ReferImage;
    CloudMask _Mask;
    // 这里采用数组分通道存储方程组是为了多线程
    std::vector<Math::LinearSystem::SparseMatrix> _A;
    std::vector<cv::Mat> _B;
    std::vector<cv::Mat> _X;
};
} // namespace RSPIP::Algorithm::ReconstructAlgorithm