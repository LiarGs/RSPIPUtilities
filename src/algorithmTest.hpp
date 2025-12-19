#pragma once
#include "RSPIP.h"
#include "Util/ImageInfoVisitor.hpp"

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
    auto normalImage = RSPIP::NormalImageRead(normalImageName);
    RSPIP::ShowImage(*normalImage);
    RSPIP::Util::PrintImageInfo(*normalImage);
    RSPIP::SaveImage(*normalImage, normalSaveImagePath, normalSaveImageName);
}

static void _TestForGeoImageMosaic() {
    auto TestImagePath = "E:/RSPIP/GuoShuai/Resource/DataWithSimuClouds/TifData/";
    auto TestMaskImagePath = "E:/RSPIP/GuoShuai/Resource/DataWithSimuClouds/MaskDatas/";
    auto imageNames = Util::GetTifImagePathFromPath(TestImagePath);
    auto cloudMaskNames = Util::GetTifImagePathFromPath(TestMaskImagePath);

    std::vector<GeoImage> imageDatas(imageNames.size());
    std::vector<CloudMask> cloudMasks(cloudMaskNames.size());

    for (int i = 0; i < imageNames.size(); ++i) {
        imageDatas[i] = *RSPIP::GeoImageRead(imageNames[i]);
    }

    for (int i = 0; i < cloudMaskNames.size(); ++i) {
        auto clodMask = RSPIP::CloudMaskImageRead(cloudMaskNames[i]);
        clodMask->InitCloudMask();
        cloudMasks[i] = std::move(*clodMask);
    }

    // mosaicAlgorithm = std::make_unique<MosaicAlgorithm::Simple>(imageDatas);
    // mosaicAlgorithm = std::make_unique<MosaicAlgorithm::ShowOverLap>(imageDatas);
    auto mosaicAlgorithm = std::make_unique<MosaicAlgorithm::DynamicPatch>(imageDatas, cloudMasks);

    SuperDebug::ScopeTimer algorithmTimer("Algorithm Execution");
    mosaicAlgorithm->Execute();
    RSPIP::SaveImage(mosaicAlgorithm->AlgorithmResult, GeoSaveImagePath, GeoSaveImageName);
}

static void _TestForColorBalance() {
    string targetImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Refer/";
    string targetImageName = "4_3_5_1204646400.tiff";

    string referImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Original/";
    string referImageName = "4_3_5_1204646400.tiff";

    string maskImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Mask/";
    string maskImageName = "4_3_5_1204646400.jpg";

    auto targetImage = RSPIP::GeoImageRead(targetImagePath + targetImageName);

    auto referImage = RSPIP::GeoImageRead(referImagePath + referImageName);

    auto maskImage = RSPIP::CloudMaskImageRead(maskImagePath + maskImageName);

    auto algorithm = std::make_unique<ColorBalanceAlgorithm::MatchStatistics>(*targetImage, *referImage, *maskImage);

    SuperDebug::ScopeTimer algorithmTimer("Algorithm Execution");
    algorithm->Execute();
    RSPIP::SaveImage(algorithm->AlgorithmResult, GeoSaveImagePath, GeoSaveImageName);
}

static void _TestForEvaluate() {
    auto resultImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Result/11_22_10_1563206400_cp/11_22_10_1563206400.tiff";
    auto referenceImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Refer/11_22_10_1563206400.tiff";
    auto maskImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Mask/11_22_10_1563206400.jpg";

    auto resultImage = RSPIP::GeoImageRead(resultImagePath);
    auto referenceImage = RSPIP::GeoImageRead(referenceImagePath);
    auto maskImage = RSPIP::CloudMaskImageRead(maskImagePath);

    auto evaluator = std::make_unique<Algorithm::BoundaryGradientEvaluator>(*resultImage, *referenceImage, *maskImage);
    evaluator->Execute();
}
