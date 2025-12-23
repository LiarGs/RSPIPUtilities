#pragma once
#include "Basic/Image.h"
#include "Interface/IGeoTransformer.h"
#include "Util/Color.h"
#include <vector>

namespace RSPIP {

class GeoImage : public Image, public IGeoTransformer {
  public:
    GeoImage();
    GeoImage(const cv::Mat &imageData);
    GeoImage(const cv::Mat &imageData, const std::string &imageName);
    virtual ~GeoImage() = default;

    void Accept(IImageVisitor &visitor) const override;

  public:
    cv::Vec3b NonData;
};

} // namespace RSPIP