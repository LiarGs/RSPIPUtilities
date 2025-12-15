// Samples For Use This Utilities
#include "RSPIP.h"
#include <Util/SuperDebug.hpp>

using namespace std;
using namespace RSPIP;
using namespace Algorithm;

auto TestImagePath = "E:/RSPIP/GuoShuai/Resource/DataWithSimuClouds/TifData/";
auto TestMaskImagePath = "E:/RSPIP/GuoShuai/Resource/DataWithSimuClouds/MaskDatas/";
auto GeoSaveImagePath = "E:/RSPIP/GuoShuai/Resource/Temp/";
auto GeoSaveImageName = "OutputImage.tif";

static void _TestForNormalImage() {
    // Test Normal Image Read/Show/Save
    auto normalImageName = "C:/Users/RSPIP/Pictures/Camera Roll/tempTest.png";
    auto normalSaveImagePath = "E:/RSPIP/GuoShuai/Resource/Temp/";
    auto normalSaveImageName = "NormalImageOutput.png";
    auto normalImage = RSPIP::ImageRead(normalImageName);
    RSPIP::ShowImage(normalImage);
    RSPIP::SaveImage(normalImage, normalSaveImagePath,
                     normalSaveImageName);
}

static void _TestForGeoImageMosaic(const auto &imageNames, const auto &cloudMaskNames) {
    std::vector<shared_ptr<RSPIP::GeoImage>> imageDatas = {};
    std::vector<shared_ptr<RSPIP::CloudMask>> cloudMasks = {};

    for (const auto &geoImagePath : imageNames) {
        auto geoImage = std::dynamic_pointer_cast<GeoImage>(RSPIP::ImageRead(geoImagePath));

        // RSPIP::ShowImage(geoImage);
        imageDatas.push_back(geoImage);
    }

    for (const auto &cloudMaskPath : cloudMaskNames) {
        auto cloudMaskImage = std::dynamic_pointer_cast<CloudMask>(RSPIP::ImageRead(cloudMaskPath, Util::ImageType::CloudMask));

        // RSPIP::ShowImage(cloudMaskImage);
        cloudMasks.push_back(cloudMaskImage);
    }

    std::unique_ptr<MosaicAlgorithm::MosaicAlgorithmBase> mosaicAlgorithm;
    std::shared_ptr<AlgorithmParamBase> algorithmParams;

    // mosaicAlgorithm = std::make_unique<MosaicAlgorithm::SimpleMosaic>();
    // mosaicAlgorithm = std::make_unique<MosaicAlgorithm::ShowOverLapMosaic>();
    mosaicAlgorithm = std::make_unique<MosaicAlgorithm::DynamicPatchMosaic>();

    // algorithmParams = std::make_shared<MosaicAlgorithm::BasicMosaicParam>(imageDatas);
    algorithmParams = std::make_shared<MosaicAlgorithm::DynamicPatchMosaicParams>(imageDatas, cloudMasks);

    // 统计算法运行时间
    SuperDebug::ScopeTimer algorithmTimer("Algorithm Execution");
    mosaicAlgorithm->Execute(algorithmParams);
    RSPIP::SaveImage(mosaicAlgorithm->MosaicResult, GeoSaveImagePath, GeoSaveImageName);
}

int main(int argc, char *argv[]) {

    SuperDebug::ScopeTimer mainTimer("Main Program");
    auto geoImagePaths = Util::GetTifImagePathFromPath(TestImagePath);

    auto cloudMaskPaths = Util::GetTifImagePathFromPath(TestMaskImagePath);

    _TestForGeoImageMosaic(geoImagePaths, cloudMaskPaths);

    return 0;
}