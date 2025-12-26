#pragma once
#include "Basic/CloudGroup.h"
#include "Basic/GeoImage.h"

namespace RSPIP {

class CloudMask : public GeoImage {
  public:
    CloudMask();
    CloudMask(const cv::Mat &imageData);
    CloudMask(const cv::Mat &cloudData, const GeoImage &sourceImage);
    CloudMask(const cv::Mat &imageData, const std::string &imageName);
    virtual ~CloudMask() = default;

    void Accept(IImageVisitor &visitor) const override;

    void Init();

    void SetSourceImage(const GeoImage &sourceImage);

  private:
    void _ExtractCloudGroups();

  public:
    const GeoImage *SourceGeoImage;
    std::vector<CloudGroup> CloudGroups;
};
} // namespace RSPIP