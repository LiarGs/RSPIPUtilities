#include "Algorithm/Reconstruct/IsophoteConstrain.h"
#include "Algorithm/Detail/AlgorithmValidation.h"
#include "Algorithm/Detail/MaskPolicies.h"
#include "Algorithm/ColorBalance/MatchStatistic.h"
#include "Math/LinearSystem/SparseSolver.h"
#include "Util/General.h"
#include "Util/SuperDebug.hpp"
#include <opencv2/imgproc.hpp>

using namespace RSPIP::Math::LinearSystem;

namespace RSPIP::Algorithm::ReconstructAlgorithm {

IsophoteConstrain::IsophoteConstrain(Image reconstructImage, Image referImage, Image maskImage)
    : _ReconstructImage(std::move(reconstructImage)), _ReferImage(std::move(referImage)), _Mask(std::move(maskImage)) {
    AlgorithmResult = _ReconstructImage;
    _A.resize(static_cast<size_t>(_ReconstructImage.GetBandCounts()));
    _B.resize(static_cast<size_t>(_ReconstructImage.GetBandCounts()));
    _X.resize(static_cast<size_t>(_ReconstructImage.GetBandCounts()));
}

void IsophoteConstrain::Execute() {
    const auto regionAnalysis = AnalyzeRegions(_Mask, Detail::DefaultBinaryMaskPolicy());
    _SelectionMask = regionAnalysis.SelectionMask;
    if (_SelectionMask.empty()) {
        Detail::ResetImageResult(AlgorithmResult);
        return;
    }

    _Regions = regionAnalysis.RegionGroups;
    if (_Regions.empty()) {
        Detail::ResetImageResult(AlgorithmResult);
        return;
    }

    ColorBalanceAlgorithm::MatchStatistics matchAlgo(_ReferImage, _ReconstructImage, _Mask);
    matchAlgo.Execute();
    if (matchAlgo.AlgorithmResult.ImageData.empty()) {
        Detail::ResetImageResult(AlgorithmResult);
        return;
    }
    _ReferImage = std::move(matchAlgo.AlgorithmResult);

    for (const auto &regionGroup : _Regions) {
        _ReconstructRegion(regionGroup);
    }
}

void IsophoteConstrain::_ReconstructRegion(const RegionGroup &regionGroup) {
#pragma omp parallel for
    for (int channelNum = 1; channelNum <= _ReconstructImage.GetBandCounts(); ++channelNum) {
        _BuildLinearSystem(regionGroup, channelNum);

        auto solver = SparseSolver(_A[static_cast<size_t>(channelNum - 1)], _B[static_cast<size_t>(channelNum - 1)], _X[static_cast<size_t>(channelNum - 1)]);
        solver.Config.Method = SolverMethod::CG;
        solver.Config.MaxIterations = _MaxIterations;
        solver.Config.Epsilon = _Epsilon;
        solver.Solve();

        for (const auto &[_, rowPixels] : regionGroup.PixelsByRow) {
            for (const auto &pixel : rowPixels) {
                const auto pixelValue = cv::saturate_cast<unsigned char>(_X[static_cast<size_t>(channelNum - 1)].at<double>(pixel.LocalIndex, 0));
                AlgorithmResult.SetPixelValue(pixel.Row, pixel.Column, channelNum, pixelValue);
            }
        }
    }
}

void IsophoteConstrain::_BuildLinearSystem(const RegionGroup &regionGroup, int channelNum) {
    const auto numPixels = regionGroup.GetNumPixels();
    _A[static_cast<size_t>(channelNum - 1)] = SparseMatrix(numPixels, numPixels);
    _B[static_cast<size_t>(channelNum - 1)] = cv::Mat::zeros(numPixels, 1, CV_64F);
    _X[static_cast<size_t>(channelNum - 1)] = cv::Mat::zeros(numPixels, 1, CV_64F);

    for (const auto &[_, rowPixels] : regionGroup.PixelsByRow) {
        for (const auto &pixel : rowPixels) {
            _BuildLinearSystemRow(regionGroup, pixel, channelNum);
        }
    }

    _A[static_cast<size_t>(channelNum - 1)].SetFromTriplets();
}

void IsophoteConstrain::_BuildLinearSystemRow(const RegionGroup &regionGroup, const RegionPixel &pixel, int channelNum) {
    const auto referPixelValue = _ReferImage.GetPixelValue<unsigned char>(pixel.Row, pixel.Column, channelNum);

    double b = 0.0;
    double sumWeight = 0.0;
    for (const auto &[dx, dy] : Util::Directions) {
        const auto neighborRow = pixel.Row + dx;
        const auto neighborColumn = pixel.Column + dy;
        if (_ReferImage.IsOutOfBounds(neighborRow, neighborColumn)) {
            continue;
        }

        const auto neighborValue = _ReferImage.GetPixelValue<unsigned char>(neighborRow, neighborColumn, channelNum);
        auto gradient = static_cast<double>(neighborValue - referPixelValue);
        if (gradient == 0.0) {
            gradient = 1.0;
        }

        const auto weight = 1.0 / (gradient * gradient);
        sumWeight += weight;
        b += weight * neighborValue;

        if (_SelectionMask.at<unsigned char>(neighborRow, neighborColumn) == 0) {
            b -= weight * _ReconstructImage.GetPixelValue<unsigned char>(neighborRow, neighborColumn, channelNum);
        } else {
            const auto &neighborPixel = regionGroup.GetPixel(neighborRow, neighborColumn);
            _A[static_cast<size_t>(channelNum - 1)].AddTriplet(pixel.LocalIndex, neighborPixel.LocalIndex, weight);
        }
    }

    b -= sumWeight * referPixelValue;
    _A[static_cast<size_t>(channelNum - 1)].AddTriplet(pixel.LocalIndex, pixel.LocalIndex, -sumWeight);
    _B[static_cast<size_t>(channelNum - 1)].at<double>(pixel.LocalIndex, 0) = b;
    _X[static_cast<size_t>(channelNum - 1)].at<double>(pixel.LocalIndex, 0) = referPixelValue;
}

} // namespace RSPIP::Algorithm::ReconstructAlgorithm
