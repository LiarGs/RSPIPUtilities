// Samples For Use This Utilities
#include "RSPIP.h"
#include "SuperDebug.hpp"

using namespace std;

int main() {
    auto exampleImagePath = "E:/RSPIP/GuoShuai/Resource/DataWithSimuClouds/"
                            "GF1B_PMS_E112.7_N23.0_20191207_L1A1227736448.tif";

    auto saveImagePath = "E:/RSPIP/GuoShuai/Resource/Temp/OutputImage.tif";
    try {
        auto img = RSPIP::ImageRead(exampleImagePath);
        if (!img) {
            Error("Unsupported image format or failed to read image.");
            return 1;
        }
        Info("Image '{}' loaded: {}x{} pixels, {} bands", img->ImageName,
             img->Height(), img->Width(), img->GetBandCounts());

        RSPIP::ShowImage(img);
        RSPIP::SaveImage(img, saveImagePath);

    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}