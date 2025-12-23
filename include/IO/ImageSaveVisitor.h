#pragma once
#include "Interface/IImageVisitor.h"
#include <string>

namespace RSPIP::IO {

class ImageSaveVisitor : public IImageVisitor {
  public:
    ImageSaveVisitor(const std::string &path, const std::string &name);

    ~ImageSaveVisitor() override = default;

    void Visit(const Image &image) override;
    void Visit(const GeoImage &geoImage) override;

    bool IsSuccess() const;

  private:
    void _SaveGeoImage(const GeoImage &geoImage);
    void _SaveNonGeoImage(const Image &image);

  private:
    std::string _Path;
    std::string _Name;
    bool _Success;
};

bool SaveImage(const Image &image, const std::string &imagePath, const std::string &imageName);

} // namespace RSPIP