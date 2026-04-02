#include "Algorithm/Mosaic/AdaptiveIsophotePatch.h"
#include "Algorithm/Reconstruct/IsophoteConstrain.h"
#include "Util/General.h"
#include "Util/SuperDebug.hpp"
#include <algorithm>
#include <limits>

namespace RSPIP::Algorithm::MosaicAlgorithm {

namespace {

constexpr unsigned char kFilledValue = 255;

cv::Vec3b _ScalarToVec3b(const cv::Scalar &value) {
    return cv::Vec3b(
        cv::saturate_cast<unsigned char>(value[0]),
        cv::saturate_cast<unsigned char>(value[1]),
        cv::saturate_cast<unsigned char>(value[2]));
}

ImageGeoBounds _BuildImageBounds(const Image &image) {
    if (image.ImageData.empty() || !image.GeoInfo.has_value() || image.GeoInfo->GeoTransform.size() < 6) {
        return image.GeoInfo.has_value() ? image.GeoInfo->Bounds : ImageGeoBounds{};
    }

    ImageGeoBounds bounds = {
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::lowest(),
        std::numeric_limits<double>::max(),
        std::numeric_limits<double>::lowest()};

    const auto updateBounds = [&](int row, int column) {
        const auto [latitude, longitude] = image.GeoInfo->GetLatLon(row, column);
        bounds.MinLongitude = std::min(bounds.MinLongitude, longitude);
        bounds.MaxLongitude = std::max(bounds.MaxLongitude, longitude);
        bounds.MinLatitude = std::min(bounds.MinLatitude, latitude);
        bounds.MaxLatitude = std::max(bounds.MaxLatitude, latitude);
    };

    const int lastRow = std::max(0, image.Height() - 1);
    const int lastColumn = std::max(0, image.Width() - 1);
    updateBounds(0, 0);
    updateBounds(0, lastColumn);
    updateBounds(lastRow, 0);
    updateBounds(lastRow, lastColumn);
    return bounds;
}

} // namespace

AdaptiveIsophotePatch::AdaptiveIsophotePatch(std::vector<Image> imageDatas, std::vector<Image> maskImages)
    : AdaptiveStripMosaicBase(std::move(imageDatas), std::move(maskImages)) {}

const char *AdaptiveIsophotePatch::_AlgorithmName() const {
    return "AdaptiveIsophotePatch";
}

AdaptiveIsophotePatch::PatchApplyResult AdaptiveIsophotePatch::_ApplyCandidatePatch(const CandidatePatch &candidate, ExpandDirection direction, int boundaryRow, int boundaryColumn) {
    if (!candidate.IsValid || candidate.InputIndex >= _Inputs.size() || candidate.Length <= 0) {
        return PatchApplyResult::Failed;
    }

    std::vector<PatchPixel> patchPixels = {};
    bool hasCompleteBoundary = false;
    if (!_BuildPatchPixels(_Inputs[candidate.InputIndex], direction, boundaryRow, boundaryColumn, candidate.Length, patchPixels, hasCompleteBoundary) || patchPixels.empty()) {
        return PatchApplyResult::Failed;
    }

    std::vector<PatchPixel> balancedPatchPixels = patchPixels;
    const bool hasBalancedFallback = hasCompleteBoundary && _BalancePatchPixelsInPlace(balancedPatchPixels);

    if (_ReconstructPatchLocally(patchPixels, candidate.InputIndex)) {
        return PatchApplyResult::PrimaryApplied;
    }

    if (hasBalancedFallback) {
        _PastePatchDirectly(balancedPatchPixels, candidate.InputIndex);
    } else {
        _PastePatchDirectly(patchPixels, candidate.InputIndex);
    }
    return PatchApplyResult::FallbackApplied;
}

bool AdaptiveIsophotePatch::_ReconstructPatchLocally(const std::vector<PatchPixel> &patchPixels, size_t ownerInputIndex) {
    if (patchPixels.empty()) {
        return false;
    }

    int minRow = std::numeric_limits<int>::max();
    int maxRow = std::numeric_limits<int>::lowest();
    int minColumn = std::numeric_limits<int>::max();
    int maxColumn = std::numeric_limits<int>::lowest();
    for (const auto &patchPixel : patchPixels) {
        minRow = std::min(minRow, patchPixel.MosaicRow);
        maxRow = std::max(maxRow, patchPixel.MosaicRow);
        minColumn = std::min(minColumn, patchPixel.MosaicColumn);
        maxColumn = std::max(maxColumn, patchPixel.MosaicColumn);
    }

    const int windowTop = std::max(0, minRow - 1);
    const int windowBottom = std::min(AlgorithmResult.Height() - 1, maxRow + 1);
    const int windowLeft = std::max(0, minColumn - 1);
    const int windowRight = std::min(AlgorithmResult.Width() - 1, maxColumn + 1);
    const cv::Rect window(windowLeft, windowTop, windowRight - windowLeft + 1, windowBottom - windowTop + 1);
    if (window.width <= 0 || window.height <= 0) {
        return false;
    }

    auto localReconstructImage = _CreateLocalImage(window);
    if (localReconstructImage.ImageData.empty()) {
        return false;
    }

    auto localReferImage = localReconstructImage;
    cv::Mat localMaskData = cv::Mat::zeros(window.height, window.width, CV_8UC1);

    for (const auto &patchPixel : patchPixels) {
        const int localRow = patchPixel.MosaicRow - window.y;
        const int localColumn = patchPixel.MosaicColumn - window.x;
        if (localRow < 0 || localRow >= window.height || localColumn < 0 || localColumn >= window.width) {
            return false;
        }

        localMaskData.at<unsigned char>(localRow, localColumn) = kFilledValue;
        localReconstructImage.SetPixelValue(localRow, localColumn, _ScalarToVec3b(localReconstructImage.GetFillScalarForOpenCV()));
        localReferImage.SetPixelValue(localRow, localColumn, patchPixel.Value);
    }

    Image localMask(localMaskData, localReconstructImage.ImageName);
    localMask.GeoInfo = localReconstructImage.GeoInfo;

    try {
        ReconstructAlgorithm::IsophoteConstrain reconstructAlgorithm(localReconstructImage, localReferImage, localMask);
        reconstructAlgorithm.SetMaxIterations(_MaxIterations);
        reconstructAlgorithm.SetEpsilon(_Epsilon);
        reconstructAlgorithm.Execute();

        for (const auto &patchPixel : patchPixels) {
            const int localRow = patchPixel.MosaicRow - window.y;
            const int localColumn = patchPixel.MosaicColumn - window.x;
            const auto reconstructedPixel = reconstructAlgorithm.AlgorithmResult.GetPixelValue<cv::Vec3b>(localRow, localColumn);
            _SetFilledPixel(
                patchPixel.MosaicRow,
                patchPixel.MosaicColumn,
                reconstructedPixel,
                static_cast<unsigned char>(Util::BGRToGray(reconstructedPixel)),
                static_cast<int>(ownerInputIndex));
        }
        return true;
    } catch (const std::exception &exception) {
        SuperDebug::Warn("AdaptiveIsophotePatch local reconstruction failed and will fallback to color-balanced direct paste when available: {}", exception.what());
        return false;
    } catch (...) {
        SuperDebug::Warn("AdaptiveIsophotePatch local reconstruction failed with an unknown error and will fallback to color-balanced direct paste when available.");
        return false;
    }
}

Image AdaptiveIsophotePatch::_CreateLocalImage(const cv::Rect &window) const {
    Image localImage = AlgorithmResult;
    localImage.ImageData = AlgorithmResult.ImageData(window).clone();
    localImage.NoDataValues = AlgorithmResult.NoDataValues;

    if (localImage.GeoInfo.has_value() && localImage.GeoInfo->GeoTransform.size() >= 6 && AlgorithmResult.GeoInfo.has_value()) {
        localImage.GeoInfo->GeoTransform[0] = AlgorithmResult.GeoInfo->GeoTransform[0] + window.x * AlgorithmResult.GeoInfo->GeoTransform[1] +
                                              window.y * AlgorithmResult.GeoInfo->GeoTransform[2];
        localImage.GeoInfo->GeoTransform[3] = AlgorithmResult.GeoInfo->GeoTransform[3] + window.x * AlgorithmResult.GeoInfo->GeoTransform[4] +
                                              window.y * AlgorithmResult.GeoInfo->GeoTransform[5];
        localImage.GeoInfo->Bounds = _BuildImageBounds(localImage);
    }

    return localImage;
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm
