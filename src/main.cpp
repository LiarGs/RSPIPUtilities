// Samples For Use This Utilities
#include "RSPIP.h"
#include <Util/SuperDebug.hpp>

using namespace std;
using namespace RSPIP;

int main() {
    auto exampleImagePath =
        "E:/RSPIP/GuoShuai/Resource/DataWithSimuClouds/TifData/";
    auto geoImageName1 = "GF1B_PMS_E112.7_N23.0_20191207_L1A1227736448.tif";
    auto geoImageName2 = "GF1B_PMS_E112.7_N23.5_20191124_L1A1227730142.tif";
    auto geoImageName3 = "GF1B_PMS_E112.8_N23.5_20191207_L1A1227736458.tif";
    auto geoImageName4 = "GF1C_PMS_E112.8_N23.0_20191208_L1A1021514557.tif";

    auto normalImageName = "C:/Users/RSPIP/Pictures/Camera Roll/tempTest.png";

    auto imageNames = vector<string>{
        geoImageName1,
        geoImageName2,
        geoImageName3,
        geoImageName4,
    };

    auto geoSaveImagePath = "E:/RSPIP/GuoShuai/Resource/Temp/";
    auto geoSaveImageName = "OutputImage.tif";
    auto normalSaveImagePath = "E:/RSPIP/GuoShuai/Resource/Temp/";
    auto normalSaveImageName = "NormalImageOutput.png";
    try {

        // Test Normal Image Read/Show/Save

        // auto normalImage = RSPIP::ImageRead(normalImageName);
        // RSPIP::ShowImage(normalImage);
        // RSPIP::SaveImage(normalImage, normalSaveImagePath,
        // normalSaveImageName);

        // Test GeoImage Read/Show/Save

        std::vector<shared_ptr<RSPIP::GeoImage>> imageDatas = {};
        for (const auto &imgName : imageNames) {
            auto imgPath = exampleImagePath + imgName;

            auto geoImage =
                std::dynamic_pointer_cast<GeoImage>(RSPIP::ImageRead(imgPath));

            // RSPIP::ShowImage(geoImage);
            imageDatas.push_back(geoImage);
        }
        std::unique_ptr<MosaicAlgorithm::IMosaicAlgorithm> mosaicUtil;

        mosaicUtil = std::make_unique<MosaicAlgorithm::DynmiacPatchMosaic>();
        // mosaicUtil = std::make_unique<MosaicAlgorithm::SimpleMosaic>();

        mosaicUtil->MosaicImages(imageDatas);
        RSPIP::SaveImage(mosaicUtil->MosaicResult, geoSaveImagePath,
                         geoSaveImageName);

    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}