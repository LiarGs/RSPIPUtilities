#pragma once

namespace RSPIP {

class Image;
class GeoImage;
class CloudMask;

class IImageVisitor {
  public:
    virtual ~IImageVisitor() = default;

    virtual void Visit(const Image &image) = 0;
    virtual void Visit(const GeoImage &image) = 0;
};

} // namespace RSPIP