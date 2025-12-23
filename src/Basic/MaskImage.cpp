#include "Basic/MaskImage.h"

namespace RSPIP {

MaskImage::MaskImage() : Image(), NonData(0) {
    ImageName = "DefaultMask.mask";
}

MaskImage::MaskImage(const cv::Mat &imageData) : Image(imageData), NonData(0) {
    ImageName = "DefaultMask.mask";
}

MaskImage::MaskImage(const cv::Mat &imageData, const std::string &imageName)
    : Image(imageData, imageName), NonData(0) {}

} // namespace RSPIP