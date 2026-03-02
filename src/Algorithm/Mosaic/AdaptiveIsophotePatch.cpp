#include "Algorithm/Mosaic/AdaptiveIsophotePatch.h"
#include "Algorithm/Reconstruct/IsophoteConstrain.h"
#include "IO/ImageSaveVisitor.h" // DebugTest
#include "Util/SortImage.hpp"
#include "Util/SuperDebug.hpp"

namespace RSPIP::Algorithm::MosaicAlgorithm {

AdaptiveIsophotePatch::AdaptiveIsophotePatch(const std::vector<GeoImage> &imageDatas, const std::vector<CloudMask> &cloudMasks)
    : MosaicAlgorithmBase(imageDatas),
      _ImageDatas(imageDatas),
      _CloudMasks(cloudMasks),
      _MosaicCloudMask() {}

void AdaptiveIsophotePatch::Execute() {
    SuperDebug::Info("Mosaic Image Size: {} x {}", AlgorithmResult.Height(), AlgorithmResult.Width());

    if (!(!_CloudMasks.empty() && _CloudMasks.size() == _ImageDatas.size())) {
        SuperDebug::Error("AdaptiveIsophotePatch requires cloud masks with the same count as input images.");
        SuperDebug::Error("Current behavior fallback: perform plain geographic mosaic without cloud-aware reconstruction.");
        for (const auto &imgData : _ImageDatas) {
            _PasteImageToMosaicResult(imgData);
        }
        SuperDebug::Info("Mosaic Completed.");
        return;
    }

    RSPIP::Util::SortImagesByLongitude(_ImageDatas);
    RSPIP::Util::SortImagesByLongitude(_CloudMasks);
    _InitCloudMasks();

    _MosaicWithoutCloud();
    _BuildMosaicCloudMask();

    _IsophoteReconstruct();

    SuperDebug::Info("Mosaic Completed.");
}

void AdaptiveIsophotePatch::_InitCloudMasks() {
    for (size_t index = 0; index < _CloudMasks.size(); ++index) {
        _CloudMasks[index].SetSourceImage(_ImageDatas[index]);
        _CloudMasks[index].Init();
    }
}

void AdaptiveIsophotePatch::_MosaicWithoutCloud() {
    cv::Mat mosaicMask = cv::Mat::zeros(AlgorithmResult.Height(), AlgorithmResult.Width(), CV_8UC1);
    _MosaicCloudMask = CloudMask(mosaicMask);
    _MosaicCloudMask.SetSourceImage(AlgorithmResult);

    for (size_t index = 0; index < _ImageDatas.size(); ++index) {
        _PasteClearPixelsToMosaicResult(_ImageDatas[index], _CloudMasks[index]);
    }
}

void AdaptiveIsophotePatch::_PasteClearPixelsToMosaicResult(const GeoImage &imageData, const CloudMask &cloudMask) {
    int columns = imageData.Width();
    int rows = imageData.Height();

    for (int row = 0; row < rows; ++row) {
        for (int col = 0; col < columns; ++col) {
            const auto &pixelValue = imageData.GetPixelValue<cv::Vec3b>(row, col);
            if (pixelValue == imageData.NonData) {
                continue;
            }

            auto [latitude, longitude] = imageData.GetLatLon(row, col);
            auto [mosaicRow, mosaicColumn] = AlgorithmResult.LatLonToRC(latitude, longitude);
            if (mosaicRow == -1 || mosaicColumn == -1) {
                continue;
            }

            // 任意一景被标记为云，则在镶嵌云掩膜中锁定该位置，并清空结果像素留给阶段二重建
            if (!cloudMask.ImageData.empty() && cloudMask.GetPixelValue<unsigned char>(row, col) != cloudMask.NonData[0]) {
                _MosaicCloudMask.SetPixelValue<unsigned char>(mosaicRow, mosaicColumn, 255);
                AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, AlgorithmResult.NonData);
                continue;
            }

            // 云并集中的位置不允许阶段一填充，避免后续影像“洗掉”云信息
            if (_MosaicCloudMask.GetPixelValue<unsigned char>(mosaicRow, mosaicColumn) != _MosaicCloudMask.NonData[0]) {
                continue;
            }

            if (AlgorithmResult.GetPixelValue<cv::Vec3b>(mosaicRow, mosaicColumn) != AlgorithmResult.NonData) {
                continue;
            }

            AlgorithmResult.SetPixelValue(mosaicRow, mosaicColumn, pixelValue);
        }
    }
}

void AdaptiveIsophotePatch::_BuildMosaicCloudMask() {
    _MosaicCloudMask.SetSourceImage(AlgorithmResult);
    _MosaicCloudMask.Init();

    // DebugTest
    auto GeoSaveImagePath = "E:/RSPIP/GuoShuai/Resource/Temp/";
    auto GeoSaveImageName = "MosaicCloudMask.tif";
    IO::SaveImage(_MosaicCloudMask, GeoSaveImagePath, GeoSaveImageName);
}

void AdaptiveIsophotePatch::_IsophoteReconstruct() {
    if (_MosaicCloudMask.CloudGroups.empty()) {
        SuperDebug::Info("No cloud region remains after stage1, skip Isophote reconstruction.");
        return;
    }

    // 阶段二关键步骤:
    // 1) 构建参考影像 referMosaic：结合 DynamicPatch 思想，从多时相中为待重建区域选择最优补丁。
    // 2) 保持 AlgorithmResult 作为 reconstructImage（目标影像），使用 _MosaicCloudMask 作为重建掩膜。
    // 3) 对每个连通云区执行 IsophoteConstrain，按通道建立并求解稀疏线性系统。
    // 4) 将求解结果回填至 AlgorithmResult 对应云区，保持非云区像素不变。
    // 5) 可选：增加颜色一致性与边界梯度评价，迭代优化参考影像与重建结果。
}

} // namespace RSPIP::Algorithm::MosaicAlgorithm
