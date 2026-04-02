#include "Basic/RegionExtraction.h"
#include "Util/SuperDebug.hpp"
#include <algorithm>
#include <opencv2/imgproc.hpp>

namespace RSPIP {

namespace {

cv::Mat _GetSelectedBandAsInt(const Image &maskImage, int band) {
    if (maskImage.ImageData.empty()) {
        SuperDebug::Error("Mask image is empty.");
        return {};
    }

    const int channels = maskImage.GetBandCounts();
    if (band <= 0 || band > channels) {
        SuperDebug::Error("Invalid mask band: {}. Available channels: {}", band, channels);
        return {};
    }

    cv::Mat selectedBand;
    if (channels == 1) {
        selectedBand = maskImage.ImageData;
    } else {
        cv::extractChannel(maskImage.ImageData, selectedBand, band - 1);
    }

    cv::Mat selectedBandInt;
    if (selectedBand.depth() == CV_32S) {
        selectedBandInt = selectedBand;
    } else {
        selectedBand.convertTo(selectedBandInt, CV_32S);
    }

    return selectedBandInt;
}

} // namespace

RegionAnalysis AnalyzeRegions(const Image &maskImage, const MaskSelectionPolicy &policy) {
    RegionAnalysis analysis = {};
    const auto selectedBand = _GetSelectedBandAsInt(maskImage, policy.Band);
    if (selectedBand.empty()) {
        return analysis;
    }

    analysis.SelectionMask = cv::Mat::zeros(selectedBand.rows, selectedBand.cols, CV_8UC1);
    switch (policy.Mode) {
    case MaskSelectionMode::NonZeroSelected:
        cv::compare(selectedBand, 0, analysis.SelectionMask, cv::CMP_NE);
        break;
    case MaskSelectionMode::ValueSetSelected:
        if (policy.SelectedValues.empty()) {
            SuperDebug::Warn("MaskSelectionPolicy::SelectedValues is empty for ValueSetSelected mode.");
            return analysis;
        }

        for (const int selectedValue : policy.SelectedValues) {
            cv::Mat currentValueMask;
            cv::compare(selectedBand, selectedValue, currentValueMask, cv::CMP_EQ);
            cv::bitwise_or(analysis.SelectionMask, currentValueMask, analysis.SelectionMask);
        }
        break;
    }

    if (analysis.SelectionMask.empty()) {
        return analysis;
    }

    cv::Mat centroids;
    const int numComponents = cv::connectedComponentsWithStats(
        analysis.SelectionMask, analysis.Labels, analysis.Stats, centroids, 8, CV_32S);

    analysis.RegionGroups.resize(static_cast<size_t>(std::max(0, numComponents - 1)));
    for (int label = 1; label < numComponents; ++label) {
        auto &regionGroup = analysis.RegionGroups[static_cast<size_t>(label - 1)];

        const int x = analysis.Stats.at<int>(label, cv::CC_STAT_LEFT);
        const int y = analysis.Stats.at<int>(label, cv::CC_STAT_TOP);
        const int width = analysis.Stats.at<int>(label, cv::CC_STAT_WIDTH);
        const int height = analysis.Stats.at<int>(label, cv::CC_STAT_HEIGHT);
        regionGroup.BoundingBox = cv::Rect(x, y, width, height);
        regionGroup.PixelCount = analysis.Stats.at<int>(label, cv::CC_STAT_AREA);
        regionGroup.LocalIndexMap = cv::Mat(height, width, CV_32SC1, cv::Scalar(-1));
        regionGroup.Pixels.reserve(static_cast<size_t>(regionGroup.PixelCount));

        int localIndex = 0;
        for (int row = y; row < y + height; ++row) {
            const int *labelPtr = analysis.Labels.ptr<int>(row);
            std::vector<RegionPixel> rowPixels = {};
            rowPixels.reserve(static_cast<size_t>(width));

            for (int column = x; column < x + width; ++column) {
                if (labelPtr[column] == label) {
                    RegionPixel pixel(row, column, localIndex++);
                    regionGroup.LocalIndexMap.at<int>(row - y, column - x) = pixel.LocalIndex;
                    regionGroup.Pixels.emplace_back(pixel);
                    rowPixels.emplace_back(pixel);
                }
            }

            if (!rowPixels.empty()) {
                regionGroup.PixelsByRow.emplace(row, std::move(rowPixels));
            }
        }
    }

    return analysis;
}

cv::Mat BuildSelectionMask(const Image &maskImage, const MaskSelectionPolicy &policy) {
    return AnalyzeRegions(maskImage, policy).SelectionMask;
}

std::vector<RegionGroup> ExtractConnectedRegions(const Image &maskImage, const MaskSelectionPolicy &policy) {
    return AnalyzeRegions(maskImage, policy).RegionGroups;
}

} // namespace RSPIP
