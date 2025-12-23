#include "Basic/GeoImage.h"
#include "Interface/IImageVisitor.h"

namespace RSPIP {

GeoImage::GeoImage() : Image(), NonData(Color::Black) {}
GeoImage::GeoImage(const cv::Mat &imageData) : Image(imageData), NonData(Color::Black) {}
GeoImage::GeoImage(const cv::Mat &imageData, const std::string &imageName) : Image(imageData, imageName), NonData(Color::Black) {}

void GeoImage::Accept(IImageVisitor &visitor) const {
    visitor.Visit(*this);
}

} // namespace RSPIP