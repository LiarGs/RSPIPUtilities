#pragma once
#include "Basic/GeoImage.h"
#include "Interface/IAlgorithm.h"
#include <vector>

namespace RSPIP::Algorithm::MosaicAlgorithm {

class MosaicAlgorithmBase : public IAlgorithm {
  protected:
    MosaicAlgorithmBase() = default;
    explicit MosaicAlgorithmBase(const std::vector<GeoImage> &imageDatas);
    MosaicAlgorithmBase(const MosaicAlgorithmBase &) = default;
    MosaicAlgorithmBase(MosaicAlgorithmBase &&) = default;
    virtual ~MosaicAlgorithmBase() = default;

    virtual void _PasteImageToMosaicResult(const GeoImage &imageData);

  private:
    virtual void _InitMosaicParameters();
    void _GetGeoInfo();
    void _GetDimensions();

  public:
    GeoImage AlgorithmResult;

  protected:
    const std::vector<GeoImage> &_MosaicImages;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm