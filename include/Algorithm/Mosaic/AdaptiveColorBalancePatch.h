#pragma once
#include "Algorithm/Mosaic/MosaicAlgorithmBase.h"
#include "Basic/CloudMask.h"
#include <array>
#include <vector>

namespace RSPIP::Algorithm::MosaicAlgorithm {

class AdaptiveColorBalancePatch : public MosaicAlgorithmBase {
  public:
    AdaptiveColorBalancePatch(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks);

    void Execute() override;

  private:
    enum class ExpandDirection {
        Up,
        Right,
        Down,
        Left
    };

    enum class PatchApplyResult {
        Failed,
        Balanced,
        DirectPaste
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

    struct PatchPixel {
        int MosaicRow = 0;
        int MosaicColumn = 0;
        cv::Vec3b CandidateValue = cv::Vec3b();
        unsigned char CandidateGray = 0;
        cv::Vec3b BoundaryValue = cv::Vec3b();
    };

    struct ExpandPassReport {
        int SegmentCount = 0;
        int SuccessStripCount = 0;
        int BalancedStripCount = 0;
        int DirectPasteStripCount = 0;
        int FailedStartCount = 0;
        int WrittenPixelCount = 0;
        double BoundaryCollectMs = 0.0;
        double CandidateEvalMs = 0.0;
        double PatchApplyMs = 0.0;

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
    PatchApplyResult _ApplyBalancedPatch(const CandidatePatch &candidate, ExpandDirection direction, int boundaryRow, int boundaryColumn);
    bool _BuildPatchPixels(const InputBundle &input, ExpandDirection direction, int boundaryRow, int boundaryColumn, int length,
                           std::vector<PatchPixel> &patchPixels, bool &hasCompleteBoundary) const;
    bool _BalancePatchPixels(const std::vector<PatchPixel> &patchPixels, std::vector<cv::Vec3b> &balancedValues) const;
    void _PastePatchDirectly(const std::vector<PatchPixel> &patchPixels, size_t ownerInputIndex);

    std::pair<int, int> _GetNormalOffset(ExpandDirection direction) const;
    std::pair<int, int> _GetTangentOffset(ExpandDirection direction) const;
    const char *_GetDirectionName(ExpandDirection direction) const;
    void _SetFilledPixel(int mosaicRow, int mosaicColumn, const cv::Vec3b &pixelValue, unsigned char grayValue, int ownerInputIndex);

  private:
    std::vector<InputBundle> _Inputs;
    cv::Mat _FilledMask;
    cv::Mat _OwnerMap;
    cv::Mat _ResultGray;
    int _FilledPixelCount = 0;
    bool _HasCompleteMaskSet = false;
    bool _HasGlobalValidStatistics = false;
    size_t _GlobalValidPixelCount = 0;
    std::array<double, 3> _GlobalValidMeans = {0.0, 0.0, 0.0};
    std::array<double, 3> _GlobalValidStdDevs = {0.0, 0.0, 0.0};
};

} // namespace RSPIP::Algorithm::MosaicAlgorithm
