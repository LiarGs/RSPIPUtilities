#pragma once
#include "Basic/Image.h"
#include "Util/Color.h"

namespace RSPIP {

class MaskImage : public Image {
  public:
    MaskImage();
    MaskImage(const cv::Mat &imageData);
    MaskImage(const cv::Mat &imageData, const std::string &imageName);
    virtual ~MaskImage() = default;

  public:
    uchar NonData;
};
} // namespace RSPIP