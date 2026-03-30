#include "Util/ProjectionTransformer.h"
#include "Util/SuperDebug.hpp"

namespace RSPIP::Util {

ProjectionTransformer::ProjectionTransformer(const std::string &sourceProjection, const std::string &targetProjection)
    : _Identity(sourceProjection.empty() || targetProjection.empty() || sourceProjection == targetProjection),
      _Valid(sourceProjection.empty() || targetProjection.empty() || sourceProjection == targetProjection),
      _SourceReference(),
      _TargetReference(),
      _Transformation(nullptr) {
    if (_Identity) {
        return;
    }

    if (_SourceReference.SetFromUserInput(sourceProjection.c_str()) != OGRERR_NONE) {
        SuperDebug::Warn("Failed to parse source projection.");
        return;
    }
    if (_TargetReference.SetFromUserInput(targetProjection.c_str()) != OGRERR_NONE) {
        SuperDebug::Warn("Failed to parse target projection.");
        return;
    }

    _SourceReference.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);
    _TargetReference.SetAxisMappingStrategy(OAMS_TRADITIONAL_GIS_ORDER);

    _Transformation.reset(OGRCreateCoordinateTransformation(&_SourceReference, &_TargetReference));
    if (_Transformation == nullptr) {
        SuperDebug::Warn("Failed to create projection transformer.");
        return;
    }

    _Valid = true;
}

bool ProjectionTransformer::IsIdentity() const {
    return _Identity;
}

bool ProjectionTransformer::IsValid() const {
    return _Valid;
}

bool ProjectionTransformer::Transform(double &latitude, double &longitude) const {
    if (_Identity) {
        return true;
    }
    if (!_Valid || _Transformation == nullptr) {
        return false;
    }

    double x = longitude;
    double y = latitude;
    if (!_Transformation->Transform(1, &x, &y, nullptr)) {
        return false;
    }

    longitude = x;
    latitude = y;
    return true;
}

void ProjectionTransformer::CoordinateTransformationDeleter::operator()(OGRCoordinateTransformation *ptr) const {
    if (ptr != nullptr) {
        OCTDestroyCoordinateTransformation(ptr);
    }
}

} // namespace RSPIP::Util
