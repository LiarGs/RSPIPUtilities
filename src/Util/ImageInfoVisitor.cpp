#include "Util/ImageInfoVisitor.h"
#include "Basic/GeoImage.h"
#include "Basic/Image.h"
#include "Util/SuperDebug.hpp"

namespace RSPIP::Util {

void ImageInfoVisitor::Visit(const Image &image) {
    _VisitImage(image);
}

void ImageInfoVisitor::Visit(const GeoImage &geoImage) {
    _VisitGeoImage(geoImage);
}

void ImageInfoVisitor::_VisitImage(const Image &image) {
    Info("Image Name: {} Dimensions: Rows: {} x Cols: {}", image.ImageName, image.Height(), image.Width());
    Info("Number of Bands: {}", image.GetBandCounts());
}

void ImageInfoVisitor::_VisitGeoImage(const GeoImage &geoImage) {
    _VisitImage(geoImage);
    geoImage.PrintGeoInfo();
}

void PrintImageInfo(const Image &image) {
    ImageInfoVisitor visitor;
    image.Accept(visitor);
}

} // namespace RSPIP::Util