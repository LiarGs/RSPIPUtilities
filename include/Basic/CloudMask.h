#pragma once
#include "Basic/CloudGroup.h"
#include "Basic/GeoImage.h"
#include "Basic/MaskImage.h"

namespace RSPIP {

class CloudMask : public GeoImage {
  public:
    CloudMask();
    CloudMask(const cv::Mat &imageData);
    CloudMask(const cv::Mat &cloudData, const GeoImage &sourceImage);
    CloudMask(const cv::Mat &imageData, const std::string &imageName);
    virtual ~CloudMask() = default;

    void Accept(IImageVisitor &visitor) const override;

    void InitCloudMask();

    void SetSourceImage(const GeoImage &sourceImage);

  private:
    void _ExtractCloudGroups();

  public:
    std::vector<CloudGroup> CloudGroups;
    const GeoImage *SourceGeoImage;
};
} // namespace RSPIP