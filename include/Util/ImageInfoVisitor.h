#pragma once
#include "Interface/IImageVisitor.h"

namespace RSPIP {
class Image;
}

namespace RSPIP::Util {

class ImageInfoVisitor : public IImageVisitor {
  public:
    void Visit(const Image &image) override;
    void Visit(const GeoImage &geoImage) override;

  private:
    void _VisitImage(const Image &image);
    void _VisitGeoImage(const GeoImage &geoImage);
};

void PrintImageInfo(const Image &image);

} // namespace RSPIP::Util