#include "Algorithm/ImageReconstruct/IsophoteConstrain.h"
#include "Algorithm/ColorBalance/MatchStatistic.h"
#include "Math/LinearSystem/SparseSolver.h"
#include "Util/General.h"
#include "Util/SuperDebug.hpp"
#include <opencv2/imgproc.hpp>
using namespace RSPIP::Math::LinearSystem;

namespace RSPIP::Algorithm::ReconstructAlgorithm {

IsophoteConstrain::IsophoteConstrain(const Image &reconstructImage, const Image &referImage, const CloudMask &maskImage)
    : _ReconstructImage(reconstructImage), _ReferImage(referImage), _Mask(maskImage) {
    AlgorithmResult = reconstructImage;
    _A.resize(reconstructImage.GetBandCounts());
    _B.resize(reconstructImage.GetBandCounts());
    _X.resize(reconstructImage.GetBandCounts());
}

void IsophoteConstrain::Execute() {
    // 先对参考影像做匀色处理
    ColorBalanceAlgorithm::MatchStatistics matchAlgo(_ReferImage, _ReconstructImage, _Mask);
    matchAlgo.Execute();
    _ReferImage = std::move(matchAlgo.AlgorithmResult);

    _Mask.Init();

    // 算法需要依次处理各个连通区域
    for (const auto &cloudGroup : _Mask.CloudGroups) {
        _ReconstructCloudGroup(cloudGroup);
    }
}

void IsophoteConstrain::_ReconstructCloudGroup(const CloudGroup &cloudGroup) {
#pragma omp parallel for
    for (int channelNum = 1; channelNum <= _ReconstructImage.GetBandCounts(); ++channelNum) {

        _BuildLinearSystem(cloudGroup, channelNum);
        Info("LinearSystem in Channel {} Build Complete", channelNum);

        auto solver = SparseSolver(_A[channelNum - 1], _B[channelNum - 1], _X[channelNum - 1]);
        solver.Config.Method = SolverMethod::CG;
        solver.Config.MaxIterations = _MaxIterations;
        solver.Config.Epsilon = _Epsilon;
        solver.Solve();

        Info("LinearSystem in Channel {} Solve Complete", channelNum);

        for (const auto &[_, rowPixels] : cloudGroup.CloudPixelMap) {
            for (const auto &pixel : rowPixels) {
                auto pixelValue = (unsigned char)_X[channelNum - 1].at<double>(pixel.PixelNumber, 0);
                AlgorithmResult.SetPixelValue(pixel.Row, pixel.Column, channelNum, pixelValue);
            }
        }
    }
}

void IsophoteConstrain::_BuildLinearSystem(const CloudGroup &cloudGroup, int channelNum) {

    auto numPixels = cloudGroup.GetNumPixels();
    _A[channelNum - 1] = SparseMatrix(numPixels, numPixels);
    _B[channelNum - 1] = cv::Mat::zeros(numPixels, 1, CV_64F);
    _X[channelNum - 1] = cv::Mat::zeros(numPixels, 1, CV_64F);

    for (const auto &[_, rowPixels] : cloudGroup.CloudPixelMap) {
        for (const auto &pixel : rowPixels) {
            _BuildLinearSystemRow(cloudGroup, pixel, channelNum);
        }
    }

    _A[channelNum - 1].SetFromTriplets();
}

void IsophoteConstrain::_BuildLinearSystemRow(const CloudGroup &cloudGroup, const CloudPixel<unsigned char> &pixel, int channelNum) {
    auto referPixelValue = _ReferImage.GetPixelValue<unsigned char>(pixel.Row, pixel.Column, channelNum);
    const auto &cloudPixelMap = cloudGroup.CloudPixelMap;

    double b = 0.0;
    double sumWeight = 0.0;
    for (const auto &[dx, dy] : Util::Directions) {
        auto neighborRow = pixel.Row + dx;
        auto neighborColumn = pixel.Column + dy;
        if (_ReferImage.IsOutOfBounds(neighborRow, neighborColumn)) { // 超出图片边界
            continue;
        }
        auto neighborValue = _ReferImage.GetPixelValue<unsigned char>(neighborRow, neighborColumn, channelNum);

        auto gradient = (double)(neighborValue - referPixelValue);
        if (gradient == 0.0) {
            gradient = 1.0;
        }
        auto weight = 1.0 / (gradient * gradient);
        sumWeight += weight;
        b += weight * neighborValue;

        // b 直接减去不在_Mask内的边界,为什么要这样做可以参考论文
        if (_Mask.ImageData.at<unsigned char>(neighborRow, neighborColumn) == _Mask.NonData[0]) { // neighbor不在_Mask内
            b -= weight * _ReconstructImage.GetPixelValue<unsigned char>(neighborRow, neighborColumn, channelNum);
        } else { // neighbor在_Mask内
            auto neighborPixelNumber = cloudGroup.GetCloudPixelByRowColumn(neighborRow, neighborColumn).PixelNumber;
            _A[channelNum - 1].AddTriplet(pixel.PixelNumber, neighborPixelNumber, weight);
        }
    }
    b -= sumWeight * referPixelValue;

    _A[channelNum - 1].AddTriplet(pixel.PixelNumber, pixel.PixelNumber, -sumWeight);
    _B[channelNum - 1].at<double>(pixel.PixelNumber, 0) = b;
    // 设定初解为参考影像像素值加快收敛速度
    _X[channelNum - 1].at<double>(pixel.PixelNumber, 0) = referPixelValue;
}

} // namespace RSPIP::Algorithm::ReconstructAlgorithm