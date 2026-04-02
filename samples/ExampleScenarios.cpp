#include "samples/ExampleScenarios.h"
#include "Math/LinearSystem/OpenCvSolver.h"
#include "Math/LinearSystem/SparseMatrix.h"
#include "Math/LinearSystem/SparseSolver.h"
#include "RSPIP.h"
#include "Util/SuperDebug.hpp"

namespace RSPIP::Samples {

namespace {

constexpr auto kGeoSaveImagePath = "E:/RSPIP/Resource/Temp/";
constexpr auto kGeoSaveImageName = "OutputImage.tif";

} // namespace

void RunNormalImageSample() {
    const auto normalImageName = "C:/Users/RSPIP/Pictures/Camera Roll/tempTest.png";
    const auto normalSaveImagePath = "E:/RSPIP/Resource/Temp/";
    const auto normalSaveImageName = "NormalImageOutput.png";
    const auto normalImage = IO::ReadImage(normalImageName);
    Util::ShowImage(normalImage);
    Util::PrintImageInfo(normalImage);
    IO::SaveImage(normalImage, normalSaveImagePath, normalSaveImageName);
}

void RunGeoRasterMosaicSample() {
    const auto testImagePath = "E:/RSPIP/Resource/DataWithSimuClouds/TifData/";
    const auto testMaskImagePath = "E:/RSPIP/Resource/DataWithSimuClouds/MaskDatas/";
    const auto imageNames = Util::GetTifImagePathFromPath(testImagePath);
    const auto cloudMaskNames = Util::GetTifImagePathFromPath(testMaskImagePath);

    std::vector<Image> imageDatas(imageNames.size());
    std::vector<Image> cloudMasks(cloudMaskNames.size());

    SuperDebug::ScopeTimer loadingTimer("Data Loading Phase");
#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(imageNames.size()); ++i) {
        imageDatas[static_cast<size_t>(i)] = IO::ReadImage(imageNames[static_cast<size_t>(i)]);
    }

#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(cloudMaskNames.size()); ++i) {
        cloudMasks[static_cast<size_t>(i)] = IO::ReadImage(cloudMaskNames[static_cast<size_t>(i)]);
    }
    loadingTimer.Stop();

    auto preprocessAlgorithm = std::make_unique<Algorithm::PreprocessAlgorithm::GeoCoordinateAlign>(imageDatas, cloudMasks);
    SuperDebug::ScopeTimer preprocessTimer("Preprocess Execution");
    preprocessAlgorithm->Execute();
    preprocessTimer.Stop();

    auto mosaicAlgorithm = std::make_unique<Algorithm::MosaicAlgorithm::AdaptiveColorBalancePatch>(
        preprocessAlgorithm->AlignedImages,
        preprocessAlgorithm->AlignedMaskImages);

    SuperDebug::ScopeTimer algorithmTimer("Algorithm Execution");
    mosaicAlgorithm->Execute();
    IO::SaveImage(mosaicAlgorithm->AlgorithmResult, kGeoSaveImagePath, kGeoSaveImageName);
}

void RunColorBalanceSample() {
    const std::string targetImagePath = "E:/RSPIP/Resource/IsoPhotoBasedReconstructionData/Refer/";
    const std::string targetImageName = "4_3_5_1204646400.tiff";

    const std::string referImagePath = "E:/RSPIP/Resource/IsoPhotoBasedReconstructionData/Original/";
    const std::string referImageName = "4_3_5_1204646400.tiff";

    const std::string maskImagePath = "E:/RSPIP/Resource/IsoPhotoBasedReconstructionData/Mask/";
    const std::string maskImageName = "4_3_5_1204646400.jpg";

    const auto targetImage = IO::ReadImage(targetImagePath + targetImageName);
    const auto referImage = IO::ReadImage(referImagePath + referImageName);
    const auto maskImage = IO::ReadImage(maskImagePath + maskImageName);

    auto algorithm = std::make_unique<Algorithm::ColorBalanceAlgorithm::MatchStatistics>(targetImage, referImage, maskImage);

    SuperDebug::ScopeTimer algorithmTimer("Algorithm Execution");
    algorithm->Execute();
    IO::SaveImage(algorithm->AlgorithmResult, kGeoSaveImagePath, kGeoSaveImageName);
}

void RunEvaluationSample() {
    const auto resultImagePath = "E:/RSPIP/Resource/IsoPhotoBasedReconstructionData/Result/11_22_10_1563206400_cp/11_22_10_1563206400.tiff";
    const auto referenceImagePath = "E:/RSPIP/Resource/IsoPhotoBasedReconstructionData/Refer/11_22_10_1563206400.tiff";
    const auto maskImagePath = "E:/RSPIP/Resource/IsoPhotoBasedReconstructionData/Mask/11_22_10_1563206400.jpg";

    const auto resultImage = IO::ReadImage(resultImagePath);
    const auto referenceImage = IO::ReadImage(referenceImagePath);
    const auto maskImage = IO::ReadImage(maskImagePath);

    auto evaluator = std::make_unique<Algorithm::BoundaryGradientEvaluator>(resultImage, referenceImage, maskImage);
    SuperDebug::ScopeTimer algorithmTimer("Algorithm Execution");
    evaluator->Execute();
    SuperDebug::Info("Result: {}", evaluator->EvaluateResult);
}

void RunReconstructSample() {
    const std::string targetImagePath = "E:/RSPIP/Resource/IsoPhotoBasedReconstructionData/Original/";
    const std::string targetImageName = "152_2_3_1456502400.tiff";

    const std::string referImagePath = "E:/RSPIP/Resource/IsoPhotoBasedReconstructionData/Refer/";
    const std::string referImageName = "152_2_3_1456502400.tiff";

    const std::string maskImagePath = "E:/RSPIP/Resource/IsoPhotoBasedReconstructionData/Mask/";
    const std::string maskImageName = "152_2_3_1456502400.jpg";

    const auto targetImage = IO::ReadImage(targetImagePath + targetImageName);
    const auto referImage = IO::ReadImage(referImagePath + referImageName);
    const auto maskImage = IO::ReadImage(maskImagePath + maskImageName);

    auto algorithm = std::make_unique<Algorithm::ReconstructAlgorithm::Simple>(targetImage, referImage, maskImage);

    SuperDebug::ScopeTimer algorithmTimer("Algorithm Execution");
    algorithm->Execute();
    IO::SaveImage(algorithm->AlgorithmResult, kGeoSaveImagePath, kGeoSaveImageName);
}

void RunCloudDetectionSample() {
    const auto testImagePath = "E:/RSPIP/Resource/DataWithSimuClouds/TifData/GF1B_PMS_E112.7_N23.0_20191207_L1A1227736448.tif";
    const auto testImage = IO::ReadImage(testImagePath);

    auto algorithm = std::make_unique<Algorithm::CloudDetectionAlgorithm::PixelThreshold>(testImage);
    SuperDebug::ScopeTimer algorithmTimer("Algorithm Execution");
    algorithm->Execute();
    IO::SaveImage(algorithm->AlgorithmResult, kGeoSaveImagePath, kGeoSaveImageName);
}

void RunLinearSolverSample() {
    using namespace Math::LinearSystem;
    SuperDebug::Info("========== Test For Linear Solver ==========");
    SuperDebug::Info("========== Test For SPD Matrix (CG Solver) ==========");

    SparseMatrix A_spd(3, 3);
    cv::Mat B_spd = cv::Mat::zeros(3, 1, CV_64F);
    cv::Mat X_spd = cv::Mat::zeros(3, 1, CV_64F);

    B_spd.at<double>(0, 0) = 9.0;
    B_spd.at<double>(1, 0) = 17.0;
    B_spd.at<double>(2, 0) = 23.0;

    A_spd.AddTriplet(0, 0, 4.0);
    A_spd.AddTriplet(0, 1, 1.0);
    A_spd.AddTriplet(0, 2, 1.0);
    A_spd.AddTriplet(1, 0, 1.0);
    A_spd.AddTriplet(1, 1, 5.0);
    A_spd.AddTriplet(1, 2, 2.0);
    A_spd.AddTriplet(2, 0, 1.0);
    A_spd.AddTriplet(2, 1, 2.0);
    A_spd.AddTriplet(2, 2, 6.0);
    A_spd.SetFromTriplets();

    SparseSolver solverSpd(A_spd, B_spd, X_spd);
    solverSpd.Config.Method = SolverMethod::CG;

    if (solverSpd.Solve()) {
        const double spdX0 = X_spd.at<double>(0, 0);
        const double spdX1 = X_spd.at<double>(1, 0);
        const double spdX2 = X_spd.at<double>(2, 0);
        SuperDebug::Info("SPD Result: x0={}, x1={}, x2={}", spdX0, spdX1, spdX2);

        if (std::abs(spdX0 - 1.0) < 1e-4 && std::abs(spdX1 - 2.0) < 1e-4 && std::abs(spdX2 - 3.0) < 1e-4) {
            SuperDebug::Info("SPD Solver Test Passed!");
        } else {
            SuperDebug::Error("SPD Solver Test Failed!");
        }
    } else {
        SuperDebug::Error("SPD Solver Failed!");
    }

    SuperDebug::Info("========== Test For General Matrix (OpenCV Solver - LU) ==========");
    cv::Mat A_dense = (cv::Mat_<double>(3, 3) << 2, 1, 1, 1, 3, 2, 1, 0, 0);
    cv::Mat B_dense = (cv::Mat_<double>(3, 1) << 4, 5, 6);
    cv::Mat X_dense;

    OpenCvSolver solverLu(A_dense, B_dense, X_dense);
    solverLu.Config.Method = SolverMethod::LU;

    if (solverLu.Solve()) {
        const double x0 = X_dense.at<double>(0, 0);
        const double x1 = X_dense.at<double>(1, 0);
        const double x2 = X_dense.at<double>(2, 0);
        SuperDebug::Info("LU Result: x0={}, x1={}, x2={}", x0, x1, x2);

        if (std::abs(x0 - 6.0) < 1e-4 && std::abs(x1 - 15.0) < 1e-4 && std::abs(x2 + 23.0) < 1e-4) {
            SuperDebug::Info("OpenCV LU Solver Test Passed!");
        } else {
            SuperDebug::Error("OpenCV LU Solver Test Failed!");
        }
    } else {
        SuperDebug::Error("OpenCV LU Solver Failed!");
    }

    SuperDebug::Info("========== Test For Over-determined System (OpenCV Solver - SVD) ==========");
    cv::Mat A_svd = (cv::Mat_<double>(3, 2) << 1, 1, 1, -1, 1, 0);
    cv::Mat B_svd = (cv::Mat_<double>(3, 1) << 2, 0, 3);
    cv::Mat X_svd;

    OpenCvSolver solverSvd(A_svd, B_svd, X_svd);
    solverSvd.Config.Method = SolverMethod::SVD;

    if (solverSvd.Solve()) {
        const double x0 = X_svd.at<double>(0, 0);
        const double x1 = X_svd.at<double>(1, 0);
        SuperDebug::Info("SVD Result: x0={}, x1={}", x0, x1);

        if (std::abs(x0 - 5.0 / 3.0) < 1e-4 && std::abs(x1 - 1.0) < 1e-4) {
            SuperDebug::Info("OpenCV SVD Solver Test Passed!");
        } else {
            SuperDebug::Error("OpenCV SVD Solver Test Failed!");
        }
    } else {
        SuperDebug::Error("OpenCV SVD Solver Failed!");
    }
}

} // namespace RSPIP::Samples
