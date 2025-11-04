// Samples For Use This Utilities
#include "RSPIP.h"
#include "SuperDebug.hpp"

using namespace std;

int main() {
    auto exampleImagePath =
        "E:/RSPIP/GuoShuai/Resource/DataWithSimuClouds/TifData/";
    auto imageName1 = "GF1B_PMS_E112.7_N23.0_20191207_L1A1227736448.tif";
    auto imageName2 = "GF1B_PMS_E112.7_N23.5_20191124_L1A1227730142.tif";
    auto imageName3 = "GF1B_PMS_E112.8_N23.5_20191207_L1A1227736458.tif";
    auto imageName4 = "GF1C_PMS_E112.8_N23.0_20191208_L1A1021514557.tif";

    auto imageNames = vector<string>{
        imageName1,
        imageName2,
        imageName3,
        imageName4,
    };

    auto saveImagePath = "E:/RSPIP/GuoShuai/Resource/Temp/OutputImage.tif";
    try {
        std::vector<shared_ptr<RSPIP::ImageData>> imageDatas = {};
        for (const auto &imgName : imageNames) {
            auto imgPath = exampleImagePath + imgName;
            auto img = RSPIP::ImageRead(imgPath);
            // RSPIP::ShowImage(img);

            imageDatas.push_back(img);
        }

        RSPIP::SimpleMosaic mosaic;
        mosaic.MosaicImages(imageDatas);

    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}