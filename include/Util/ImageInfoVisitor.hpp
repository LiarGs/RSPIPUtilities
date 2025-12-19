#pragma once
#include "Basic/CloudMask.hpp"
#include "Interface/IImageVisitor.hpp"
#include "Util/SuperDebug.hpp"

namespace RSPIP::Util {

class ImageInfoVisitor : public IImageVisitor {
  public:
    void Visit(const Image &image) override {
        Info("Image Name: {} Dimensions: Rows: {} x Cols: {}", image.ImageName, image.Height(), image.Width());
        Info("Number of Bands: {}", image.GetBandCounts());
    }
    void Visit(const GeoImage &geoImage) override {
        Visit(static_cast<const Image &>(geoImage));
        geoImage.PrintGeoInfo();
    }

    void Visit(const CloudMask &cloudMask) override {
        Visit(static_cast<const GeoImage &>(cloudMask));
    }
};

inline void PrintImageInfo(const Image &image) {
    ImageInfoVisitor visitor;
    image.Accept(visitor);
}

} // namespace RSPIP::Util
