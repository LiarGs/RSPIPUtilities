#pragma once
#include "Math/LinearSystem/OpenCvSolver.h"
#include "Math/LinearSystem/SparseMatrix.h"
#include "Math/LinearSystem/SparseSolver.h"
#include "RSPIP.h"
#include "Util/SuperDebug.hpp"

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

    SuperDebug::ScopeTimer loadingTimer("Data Loading Phase");
#pragma omp parallel for
    for (int i = 0; i < imageNames.size(); ++i) {
        imageDatas[i] = std::move(*IO::GeoImageRead(imageNames[i]));
    }

#pragma omp parallel for
    for (int i = 0; i < cloudMaskNames.size(); ++i) {
        auto clodMask = IO::CloudMaskImageRead(cloudMaskNames[i]);
        cloudMasks[i] = std::move(*clodMask);
    }

    loadingTimer.Stop();

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

static void _TestForReconstruct() {
    std::string targetImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Original/";
    std::string targetImageName = "11_22_10_1563206400.tiff";

    std::string referImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Refer/";
    std::string referImageName = "11_22_10_1563206400.tiff";

    std::string maskImagePath = "E:/RSPIP/GuoShuai/IsoPhotoBasedReconstruction/data/Mask/";
    std::string maskImageName = "11_22_10_1563206400.jpg";

    // std::string maskImagePath = "E:/RSPIP/GuoShuai/Resource/Temp/";
    // std::string maskImageName = "Debug.jpg";

    auto targetImage = IO::GeoImageRead(targetImagePath + targetImageName);

    auto referImage = IO::GeoImageRead(referImagePath + referImageName);

    auto maskImage = IO::CloudMaskImageRead(maskImagePath + maskImageName);

    // auto algorithm = std::make_unique<ReconstructAlgorithm::Simple>(*targetImage, *referImage, *maskImage);
    // auto algorithm = std::make_unique<ReconstructAlgorithm::ColorBalanceReconstruct>(*targetImage, *referImage, *maskImage);
    auto algorithm = std::make_unique<ReconstructAlgorithm::IsophoteConstrain>(*targetImage, *referImage, *maskImage);

    SuperDebug::ScopeTimer algorithmTimer("Algorithm Execution");
    algorithm->Execute();
    IO::SaveImage(algorithm->AlgorithmResult, GeoSaveImagePath, GeoSaveImageName);
}

static void _TestForLinearSolver() {
    using namespace Math::LinearSystem;
    SuperDebug::Info("========== Test For Linear Solver ==========");
    SuperDebug::Info("========== Test For SPD Matrix (CG Solver) ==========");
    // Test Case: Solve 3x3 Symmetric Positive Definite Matrix using CG
    // Matrix A:
    // 4 1 1
    // 1 5 2
    // 1 2 6
    // Expected Solution: x0 = 1.0, x1 = 2.0, x2 = 3.0

    SparseMatrix A_spd(3, 3);
    cv::Mat B_spd = cv::Mat::zeros(3, 1, CV_64F);
    cv::Mat X_spd = cv::Mat::zeros(3, 1, CV_64F);

    // Setup B
    B_spd.at<double>(0, 0) = 9.0;
    B_spd.at<double>(1, 0) = 17.0;
    B_spd.at<double>(2, 0) = 23.0;

    // Setup A (Symmetric)
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

    // Solve using CG
    SparseSolver solver_spd(A_spd, B_spd, X_spd);
    solver_spd.Config.Method = SolverMethod::CG;

    if (solver_spd.Solve()) {
        double spd_x0 = X_spd.at<double>(0, 0);
        double spd_x1 = X_spd.at<double>(1, 0);
        double spd_x2 = X_spd.at<double>(2, 0);
        SuperDebug::Info("SPD Result: x0={}, x1={}, x2={}", spd_x0, spd_x1, spd_x2);

        if (std::abs(spd_x0 - 1.0) < 1e-4 && std::abs(spd_x1 - 2.0) < 1e-4 && std::abs(spd_x2 - 3.0) < 1e-4) {
            SuperDebug::Info("SPD Solver Test Passed!");
        } else {
            SuperDebug::Error("SPD Solver Test Failed!");
        }
    } else {
        SuperDebug::Error("SPD Solver Failed!");
    }

    SuperDebug::Info("========== Test For General Matrix (OpenCV Solver - LU) ==========");
    // Test Case: Solve 3x3 Linear System
    // 2x + y + z = 4
    // x + 3y + 2z = 5
    // x = 6
    // Expected: x=6, y=15, z=-23

    cv::Mat A_dense = (cv::Mat_<double>(3, 3) << 2, 1, 1, 1, 3, 2, 1, 0, 0);
    cv::Mat B_dense = (cv::Mat_<double>(3, 1) << 4, 5, 6);
    cv::Mat X_dense;

    OpenCvSolver solver_lu(A_dense, B_dense, X_dense);
    solver_lu.Config.Method = SolverMethod::LU;

    if (solver_lu.Solve()) {
        double x0 = X_dense.at<double>(0, 0);
        double x1 = X_dense.at<double>(1, 0);
        double x2 = X_dense.at<double>(2, 0);
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
    // Test Case: Least Squares Problem
    // x + y = 2
    // x - y = 0
    // x = 3
    // Least Squares Solution: x=5/3 (~1.6667), y=1.0

    cv::Mat A_svd = (cv::Mat_<double>(3, 2) << 1, 1, 1, -1, 1, 0);
    cv::Mat B_svd = (cv::Mat_<double>(3, 1) << 2, 0, 3);
    cv::Mat X_svd;

    OpenCvSolver solver_svd(A_svd, B_svd, X_svd);
    solver_svd.Config.Method = SolverMethod::SVD;

    if (solver_svd.Solve()) {
        double x0 = X_svd.at<double>(0, 0);
        double x1 = X_svd.at<double>(1, 0);
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