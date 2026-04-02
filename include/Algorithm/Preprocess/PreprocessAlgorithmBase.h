#pragma once
#include "Basic/Image.h"
#include "Interface/IAlgorithm.h"
#include <string>
#include <utility>
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
    explicit PreprocessAlgorithmBase(std::vector<Image> imageDatas)
        : _ImageDatas(std::move(imageDatas)), _MaskImages() {}
    PreprocessAlgorithmBase(std::vector<Image> imageDatas, std::vector<Image> maskImages)
        : _ImageDatas(std::move(imageDatas)), _MaskImages(std::move(maskImages)) {}
    virtual ~PreprocessAlgorithmBase() = default;

  protected:
    PreprocessAlgorithmBase() = default;
    PreprocessAlgorithmBase(const PreprocessAlgorithmBase &) = default;
    PreprocessAlgorithmBase(PreprocessAlgorithmBase &&) = default;

  public:
    UnifiedGridInfo UnifiedGrid;
    std::vector<Image> AlignedImages;
    std::vector<Image> AlignedMaskImages;

  protected:
    std::vector<Image> _ImageDatas;
    std::vector<Image> _MaskImages;
};

} // namespace RSPIP::Algorithm::PreprocessAlgorithm
