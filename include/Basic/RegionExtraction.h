#pragma once
#include "Basic/Image.h"
#include "Basic/MaskSelectionPolicy.h"
#include "Basic/RegionGroup.h"
#include <vector>

namespace RSPIP {

struct RegionAnalysis {
  public:
    cv::Mat SelectionMask;
    cv::Mat Labels;
    cv::Mat Stats;
    std::vector<RegionGroup> RegionGroups;
};

RegionAnalysis AnalyzeRegions(const Image &maskImage, const MaskSelectionPolicy &policy = {});
cv::Mat BuildSelectionMask(const Image &maskImage, const MaskSelectionPolicy &policy = {});
std::vector<RegionGroup> ExtractConnectedRegions(const Image &maskImage, const MaskSelectionPolicy &policy = {});

} // namespace RSPIP
