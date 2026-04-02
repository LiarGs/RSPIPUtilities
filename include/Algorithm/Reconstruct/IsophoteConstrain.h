/// Inspired by paper
/// Missing information reconstruction integrating isophote constraint and color-structure control for remote sensing data
/// by YU Xiao Yu

#pragma once
#include "Algorithm/Reconstruct/ReconstructAlgorithmBase.h"
#include "Basic/RegionExtraction.h"
#include "Math/LinearSystem/SparseMatrix.h"

namespace RSPIP::Algorithm::ReconstructAlgorithm {

class IsophoteConstrain : public ReconstructAlgorithmBase {
  public:
    IsophoteConstrain(Image reconstructImage, Image referImage, Image maskImage);
    void Execute() override;

    void SetMaxIterations(int newMaxIterations) {
        _MaxIterations = newMaxIterations;
    }

    void SetEpsilon(double newEpsilon) {
        _Epsilon = newEpsilon;
    }

  private:
    void _ReconstructRegion(const RegionGroup &regionGroup);
    void _BuildLinearSystem(const RegionGroup &regionGroup, int channelNum);
    void _BuildLinearSystemRow(const RegionGroup &regionGroup, const RegionPixel &pixel, int channelNum);

  private:
    Image _ReconstructImage;
    Image _ReferImage;
    Image _Mask;
    cv::Mat _SelectionMask;
    std::vector<RegionGroup> _Regions;
    std::vector<Math::LinearSystem::SparseMatrix> _A;
    std::vector<cv::Mat> _B;
    std::vector<cv::Mat> _X;

    int _MaxIterations = 10000;
    double _Epsilon = 1;
};

} // namespace RSPIP::Algorithm::ReconstructAlgorithm
