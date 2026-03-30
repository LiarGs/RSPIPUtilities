#pragma once
#include <memory>
#include <ogr_spatialref.h>
#include <string>

namespace RSPIP::Util {

class ProjectionTransformer {
  public:
    ProjectionTransformer(const std::string &sourceProjection, const std::string &targetProjection);

    bool IsIdentity() const;
    bool IsValid() const;
    bool Transform(double &latitude, double &longitude) const;

  private:
    struct CoordinateTransformationDeleter {
        void operator()(OGRCoordinateTransformation *ptr) const;
    };

  private:
    bool _Identity;
    bool _Valid;
    OGRSpatialReference _SourceReference;
    OGRSpatialReference _TargetReference;
    std::unique_ptr<OGRCoordinateTransformation, CoordinateTransformationDeleter> _Transformation;
};

} // namespace RSPIP::Util
