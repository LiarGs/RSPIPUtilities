#include "Algorithm/ImageReconstruct/IsophoteConstrain.h"
#include "Algorithm/ColorBalance/MatchStatistic.h"
#include "Math/LinearSystem/SparseSolver.h"
#include "Util/General.h"
#include "Util/SuperDebug.hpp"
#include <opencv2/imgproc.hpp>
using namespace RSPIP::Math::LinearSystem;

namespace RSPIP::Algorithm::ReconstructAlgorithm {

IsophoteConstrain::IsophoteConstrain(const Image &reconstructImage, const Image &referImage, const CloudMask &maskImage)
    : _ReconstructImage(reconstructImage), _Mask(maskImage) { AlgorithmResult = reconstructImage; }

void IsophoteConstrain::Execute() {
    // 先对参考影像做匀色处理
    ColorBalanceAlgorithm::MatchStatistics matchAlgo(_ReferImage, _ReconstructImage, _Mask);
    matchAlgo.Execute();
    _ReferImage = std::move(matchAlgo.AlgorithmResult);

    // 扩展连通区域边界
    auto kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::dilate(_Mask.ImageData, _Mask.ImageData, kernel);
    _Mask.Init();

    Info("ConnectArea : {}", _Mask.CloudGroups.size());
    // 算法需要依次处理各个连通区域
    for (const auto &cloudGroup : _Mask.CloudGroups) {
        _ReconstructCloudGroup(cloudGroup);
    }
}

void IsophoteConstrain::_ReconstructCloudGroup(const CloudGroup &cloudGroup) {
    // #pragma omp parallel for
    for (int channelNum = 1; channelNum <= _ReconstructImage.GetBandCounts(); ++channelNum) {

        auto [a, b, x] = _BuildLinearSystem(cloudGroup, channelNum);

        auto solver = SparseSolver(a, b, x);
        solver.Solve();

        for (const auto &[_, rowPixels] : cloudGroup.CloudPixelMap) {
            for (const auto &pixel : rowPixels) {
                auto pixelValue = (unsigned char)x.at<double>(pixel.PixelNumber, 0);
                AlgorithmResult.SetPixelValue(pixel.Row, pixel.Column, channelNum, pixelValue);
            }
        }
    }
}

std::tuple<SparseMatrix, cv::Mat, cv::Mat> IsophoteConstrain::_BuildLinearSystem(const CloudGroup &cloudGroup, int channelNum) {

    auto numPixels = cloudGroup.GetNumPixels();
    auto a = SparseMatrix(numPixels, numPixels);
    cv::Mat x = cv::Mat::zeros(numPixels, 1, CV_64F);
    cv::Mat b = cv::Mat::zeros(numPixels, 1, CV_64F);

    std::vector<Triplet> triplets;
    triplets.reserve(numPixels);

    for (const auto &[_, rowPixels] : cloudGroup.CloudPixelMap) {
        for (const auto &pixel : rowPixels) {
            _BuildLinearSystemRow(pixel, channelNum);
            // 设定初解为参考影像像素值加快收敛速度
            x.at<double>(pixel.PixelNumber, 0) = _ReferImage.GetPixelValue<unsigned char>(pixel.Row, pixel.Column, channelNum);
        }
    }

    a.SetFromTriplets(triplets);

    return std::tuple{a, b, x};
}

void IsophoteConstrain::_BuildLinearSystemRow(const CloudPixel<unsigned char> &pixel, int channelNum) {

    auto referPixelValue = _ReferImage.GetPixelValue<unsigned char>(pixel.Row, pixel.Column, 1);
    double b = 0.0;

    for (const auto [dx, dy] : Util::Directions) {
        auto neighborRow = pixel.Row + dx;
        auto neighborCol = pixel.Column + dy;
        auto neighborValue = _ReferImage.GetPixelValue<unsigned char>(neighborRow, neighborCol, channelNum);
        if (neighborValue == -1) { // 越界
            continue;
        }

        auto gradient = (neighborValue - referPixelValue) == 0 ? 1 : (neighborValue - referPixelValue);
        auto weight = 1 / gradient * gradient;
        b += 1 / gradient;
        // b 直接减去不在_Mask内的边界
        if (_Mask.ImageData.at<unsigned char>(neighborRow, neighborCol) == _Mask.NonData[0]) { // 不在_Mask内
            b -= weight * _ReconstructImage.GetPixelValue<unsigned char>(neighborRow, neighborCol, channelNum);
        }
    }
}

} // namespace RSPIP::Algorithm::ReconstructAlgorithm