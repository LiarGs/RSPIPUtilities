#pragma once
#include "Basic/CloudMask.h"
#include "Basic/GeoImage.h"
#include "Interface/IAlgorithm.h"
#include <string>
#include <vector>

namespace RSPIP::Algorithm::PreprocessAlgorithm {

struct UnifiedGridInfo {
    std::vector<double> GeoTransform;
    std::string Projection;
    int Rows = 0;
    int Columns = 0;
};

class PreprocessAlgorithmBase : public IAlgorithm {
  public:
    explicit PreprocessAlgorithmBase(const std::vector<GeoImage> &imageDatas)
        : _ImageDatas(imageDatas), _OwnedCloudMasks(), _CloudMasks(_OwnedCloudMasks) {}
    PreprocessAlgorithmBase(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks)
        : _ImageDatas(imageDatas), _OwnedCloudMasks(), _CloudMasks(cloudMasks) {}
    virtual ~PreprocessAlgorithmBase() = default;

  protected:
    PreprocessAlgorithmBase() = default;
    PreprocessAlgorithmBase(const PreprocessAlgorithmBase &) = default;
    PreprocessAlgorithmBase(PreprocessAlgorithmBase &&) = default;

  public:
    UnifiedGridInfo UnifiedGrid;
    std::vector<GeoImage> AlignedImages;
    std::vector<CloudMask> AlignedCloudMasks;

  protected:
    const std::vector<GeoImage> &_ImageDatas;
    std::vector<CloudMask> _OwnedCloudMasks;
    const std::vector<CloudMask> &_CloudMasks;
};

} // namespace RSPIP::Algorithm::PreprocessAlgorithm
