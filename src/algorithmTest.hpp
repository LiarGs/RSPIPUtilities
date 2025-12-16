#pragma once
#include "RSPIP.h"
#include <Util/SuperDebug.hpp>

using namespace std;
using namespace RSPIP;
using namespace Algorithm;

auto GeoSaveImagePath = "E:/RSPIP/GuoShuai/Resource/Temp/";
auto GeoSaveImageName = "OutputImage.tif";

static void _TestForNormalImage() {
    // Test Normal Image Read/Show/Save
    auto normalImageName = "C:/Users/RSPIP/Pictures/Camera Roll/tempTest.png";
    auto normalSaveImagePath = "E:/RSPIP/GuoShuai/Resource/Temp/";
    auto normalSaveImageName = "NormalImageOutput.png";
    auto normalImage = RSPIP::ImageRead(normalImageName, Util::ImageType::NormalImage);
    RSPIP::ShowImage(normalImage);
    RSPIP::SaveImage(normalImage, normalSaveImagePath,
                     normalSaveImageName);
}

static void _TestForGeoImageMosaic() {
    auto TestImagePath = "E:/RSPIP/GuoShuai/Resource/DataWithSimuClouds/TifData/";
    auto TestMaskImagePath = "E:/RSPIP/GuoShuai/Resource/DataWithSimuClouds/MaskDatas/";
    auto imageNames = Util::GetTifImagePathFromPath(TestImagePath);
    auto cloudMaskNames = Util::GetTifImagePathFromPath(TestMaskImagePath);

    std::vector<shared_ptr<RSPIP::GeoImage>> imageDatas = {};
    std::vector<shared_ptr<RSPIP::CloudMask>> cloudMasks = {};

    for (const auto &geoImagePath : imageNames) {
        auto geoImage = std::dynamic_pointer_cast<GeoImage>(RSPIP::ImageRead(geoImagePath, Util::ImageType::GeoImage));

        // RSPIP::ShowImage(geoImage);
        imageDatas.push_back(geoImage);
    }

    for (const auto &cloudMaskPath : cloudMaskNames) {
        auto cloudMaskImage = std::dynamic_pointer_cast<CloudMask>(RSPIP::ImageRead(cloudMaskPath, Util::ImageType::CloudMask));

        // RSPIP::ShowImage(cloudMaskImage);
        cloudMasks.push_back(cloudMaskImage);
    }

    std::unique_ptr<Algorithm::IAlgorithm> mosaicAlgorithm;
    std::shared_ptr<AlgorithmParamBase> algorithmParams;

    // mosaicAlgorithm = std::make_unique<MosaicAlgorithm::Simple>();
    // mosaicAlgorithm = std::make_unique<MosaicAlgorithm::ShowOverLap>();
    mosaicAlgorithm = std::make_unique<MosaicAlgorithm::DynamicPatch>();

    // algorithmParams = std::make_shared<MosaicAlgorithm::BasicMosaicParam>(imageDatas);
    algorithmParams = std::make_shared<MosaicAlgorithm::DynamicPatchParams>(imageDatas, cloudMasks);

    // 统计算法运行时间
    SuperDebug::ScopeTimer algorithmTimer("Algorithm Execution");
    mosaicAlgorithm->Execute(algorithmParams);
    RSPIP::SaveImage(mosaicAlgorithm->AlgorithmResult, GeoSaveImagePath, GeoSaveImageName);
}

static void _TestForColorBalance() {
    string targetImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Refer/";
    string targetImageName = "4_3_5_1204646400.tiff";

    string referImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Original/";
    string referImageName = "4_3_5_1204646400.tiff";

    string maskImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Mask/";
    string maskImageName = "4_3_5_1204646400.jpg";

    auto targetImage = std::dynamic_pointer_cast<GeoImage>(RSPIP::ImageRead(targetImagePath + targetImageName, Util::ImageType::GeoImage));
    auto referImage = std::dynamic_pointer_cast<GeoImage>(RSPIP::ImageRead(referImagePath + referImageName, Util::ImageType::GeoImage));
    auto maskImage = std::dynamic_pointer_cast<CloudMask>(RSPIP::ImageRead(maskImagePath + maskImageName, Util::ImageType::CloudMask));

    std::unique_ptr<Algorithm::IAlgorithm> algorithm;
    std::shared_ptr<AlgorithmParamBase> algorithmParams;

    algorithm = std::make_unique<ColorBalanceAlgorithm::MatchStatistics>();

    algorithmParams = std::make_shared<ColorBalanceAlgorithm::MatchStatisticsParams>(targetImage, referImage, maskImage);

    // 统计算法运行时间
    SuperDebug::ScopeTimer algorithmTimer("Algorithm Execution");
    algorithm->Execute(algorithmParams);
    RSPIP::SaveImage(algorithm->AlgorithmResult, GeoSaveImagePath, GeoSaveImageName);
}
