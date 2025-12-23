#pragma once
#include "RSPIP.h"
#include "Util/ImageInfoVisitor.h"
#include <Util/SuperDebug.hpp>

using namespace RSPIP;
using namespace Algorithm;

auto GeoSaveImagePath = "E:/RSPIP/GuoShuai/Resource/Temp/";
auto GeoSaveImageName = "OutputImage.tif";

static void _TestForNormalImage() {
    // Test Normal Image Read/Show/Save
    auto normalImageName = "C:/Users/RSPIP/Pictures/Camera Roll/tempTest.png";
    auto normalSaveImagePath = "E:/RSPIP/GuoShuai/Resource/Temp/";
    auto normalSaveImageName = "NormalImageOutput.png";
    auto normalImage = IO::NormalImageRead(normalImageName);
    Util::ShowImage(*normalImage);
    Util::PrintImageInfo(*normalImage);
    IO::SaveImage(*normalImage, normalSaveImagePath, normalSaveImageName);
}

static void _TestForGeoImageMosaic() {
    auto TestImagePath = "E:/RSPIP/GuoShuai/Resource/DataWithSimuClouds/TifData/";
    auto TestMaskImagePath = "E:/RSPIP/GuoShuai/Resource/DataWithSimuClouds/MaskDatas/";
    auto imageNames = Util::GetTifImagePathFromPath(TestImagePath);
    auto cloudMaskNames = Util::GetTifImagePathFromPath(TestMaskImagePath);

    std::vector<GeoImage> imageDatas(imageNames.size());
    std::vector<CloudMask> cloudMasks(cloudMaskNames.size());

#pragma omp parallel for
    for (int i = 0; i < imageNames.size(); ++i) {
        imageDatas[i] = std::move(*IO::GeoImageRead(imageNames[i]));
    }

#pragma omp parallel for
    for (int i = 0; i < cloudMaskNames.size(); ++i) {
        auto clodMask = IO::CloudMaskImageRead(cloudMaskNames[i]);
        clodMask->InitCloudMask();
        cloudMasks[i] = std::move(*clodMask);
    }

    // auto mosaicAlgorithm = std::make_unique<MosaicAlgorithm::Simple>(imageDatas);
    // auto mosaicAlgorithm = std::make_unique<MosaicAlgorithm::ShowOverLap>(imageDatas);
    auto mosaicAlgorithm = std::make_unique<MosaicAlgorithm::DynamicPatch>(imageDatas, cloudMasks);

    SuperDebug::ScopeTimer algorithmTimer("Algorithm Execution");
    mosaicAlgorithm->Execute();
    IO::SaveImage(mosaicAlgorithm->AlgorithmResult, GeoSaveImagePath, GeoSaveImageName);
}

static void _TestForColorBalance() {
    std::string targetImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Refer/";
    std::string targetImageName = "4_3_5_1204646400.tiff";

    std::string referImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Original/";
    std::string referImageName = "4_3_5_1204646400.tiff";

    std::string maskImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Mask/";
    std::string maskImageName = "4_3_5_1204646400.jpg";

    auto targetImage = IO::GeoImageRead(targetImagePath + targetImageName);

    auto referImage = IO::GeoImageRead(referImagePath + referImageName);

    auto maskImage = IO::CloudMaskImageRead(maskImagePath + maskImageName);

    auto algorithm = std::make_unique<ColorBalanceAlgorithm::MatchStatistics>(*targetImage, *referImage, *maskImage);

    SuperDebug::ScopeTimer algorithmTimer("Algorithm Execution");
    algorithm->Execute();
    IO::SaveImage(algorithm->AlgorithmResult, GeoSaveImagePath, GeoSaveImageName);
}

static void _TestForEvaluate() {
    auto resultImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Result/11_22_10_1563206400_cp/11_22_10_1563206400.tiff";
    auto referenceImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Refer/11_22_10_1563206400.tiff";
    auto maskImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Mask/11_22_10_1563206400.jpg";

    auto resultImage = IO::GeoImageRead(resultImagePath);
    auto referenceImage = IO::GeoImageRead(referenceImagePath);
    auto maskImage = IO::CloudMaskImageRead(maskImagePath);

    auto evaluator = std::make_unique<Algorithm::BoundaryGradientEvaluator>(*resultImage, *referenceImage, *maskImage);
    SuperDebug::ScopeTimer algorithmTimer("Algorithm Execution");
    evaluator->Execute();
    SuperDebug::Info("Result: {}", evaluator->EvaluateResult);
}
