#pragma once
#include "Basic/Image.h"
#include "Interface/IAlgorithm.h"
#include <vector>

namespace RSPIP::Algorithm::MosaicAlgorithm {

class MosaicAlgorithmBase : public IAlgorithm {
  public:
    virtual ~MosaicAlgorithmBase() = default;

  protected:
    MosaicAlgorithmBase() = default;
    explicit MosaicAlgorithmBase(std::vector<Image> imageDatas);
    MosaicAlgorithmBase(const MosaicAlgorithmBase &) = default;
    MosaicAlgorithmBase(MosaicAlgorithmBase &&) = default;

    virtual void _PasteImageToMosaicResult(const Image &imageData);

  private:
    void _InitMosaicParameters();
    bool _GetGeoInfo();
    void _GetDimensions();

  public:
    Image AlgorithmResult;

  protected:
    std::vector<Image> _MosaicImages;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm
