#pragma once
#include "Image.hpp"
#include "Util/Color.hpp"

namespace RSPIP {

class MaskImage : public Image {

  public:
    MaskImage() : Image(), NonData(0) { ImageName = "DefaultMask.mask"; }

    MaskImage(const cv::Mat &imageData) : Image(imageData), NonData(0) { ImageName = "DefaultMask.mask"; }

    MaskImage(const cv::Mat &imageData, const std::string &imageName) : Image(imageData, imageName), NonData(0) {}

  public:
    uchar NonData;
};
} // namespace RSPIP