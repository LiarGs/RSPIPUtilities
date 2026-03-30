#pragma once
#include "Algorithm/Mosaic/MosaicAlgorithmBase.h"
#include "Basic/CloudMask.h"
#include <vector>

namespace RSPIP::Algorithm::MosaicAlgorithm {

class AdaptivePatch : public MosaicAlgorithmBase {
  public:
    AdaptivePatch(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks);

    void Execute() override;

  private:
    enum class ExpandDirection {
        Up,
        Right,
        Down,
        Left
    };

    struct InputBundle {
        GeoImage Image;
        CloudMask Mask;
        cv::Mat ValidMask;
        cv::Mat GrayImage;
        double ClearRatio = 0.0;
        size_t OriginalIndex = 0;
        int RowOffset = 0;
        int ColumnOffset = 0;
    };

    struct SeedRegion {
        size_t InputIndex = 0;
        cv::Rect Rectangle;
        double ClearRatio = 0.0;

        bool IsValid() const {
            return Rectangle.width > 0 && Rectangle.height > 0;
        }
    };

    struct BoundarySegment {
        int StartRow = 0;
        int StartColumn = 0;
        int Length = 0;
    };

    struct CandidatePatch {
        size_t InputIndex = 0;
        int Length = 0;
        double Correlation = 0.0;
        bool IsValid = false;
    };

    struct ExpandPassReport {
        int SegmentCount = 0;
        int SuccessStripCount = 0;
        int FailedStartCount = 0;
        int WrittenPixelCount = 0;
        double BoundaryCollectMs = 0.0;
        double CandidateEvalMs = 0.0;
        double PatchCopyMs = 0.0;

        bool Expanded() const {
            return WrittenPixelCount > 0;
        }
    };

  private:
    void _PrepareInputs();
    void _FallbackToPlainMosaic();
    bool _InitializeSeedRegion();
    cv::Rect _FindLargestValidRectangle(const cv::Mat &validMask) const;

    ExpandPassReport _ExpandDirection(ExpandDirection direction);
    std::vector<BoundarySegment> _CollectBoundarySegments(ExpandDirection direction) const;
    bool _IsBoundaryPixel(ExpandDirection direction, int row, int column) const;
    bool _IsFilled(int row, int column) const;

    CandidatePatch _SelectBestCandidate(ExpandDirection direction, int boundaryRow, int boundaryColumn, int maxLength) const;
    CandidatePatch _EvaluateCandidate(size_t inputIndex, const InputBundle &input, ExpandDirection direction, int boundaryRow, int boundaryColumn, int maxLength) const;
    void _CopyPatchToResult(const InputBundle &input, ExpandDirection direction, int boundaryRow, int boundaryColumn, int length);

    std::pair<int, int> _GetNormalOffset(ExpandDirection direction) const;
    std::pair<int, int> _GetTangentOffset(ExpandDirection direction) const;
    const char *_GetDirectionName(ExpandDirection direction) const;
    void _SetFilledPixel(int mosaicRow, int mosaicColumn, const cv::Vec3b &pixelValue, unsigned char grayValue);

  private:
    std::vector<InputBundle> _Inputs;
    cv::Mat _FilledMask;
    cv::Mat _ResultGray;
    int _FilledPixelCount = 0;
    long long _LegacyGeoMapCallCount = 0;
    bool _HasCompleteMaskSet = false;
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm
