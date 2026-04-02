#include "samples/ExampleScenarios.h"
#include "Util/SuperDebug.hpp"
#include <gdal_priv.h>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    SuperDebug::ScopeTimer mainTimer("Main Program");
    GDALAllRegister();

    // RSPIP::Samples::RunNormalImageSample();
    // RSPIP::Samples::RunGeoRasterMosaicSample();
    // RSPIP::Samples::RunColorBalanceSample();
    // RSPIP::Samples::RunEvaluationSample();
    // RSPIP::Samples::RunReconstructSample();
    // RSPIP::Samples::RunCloudDetectionSample();
    // RSPIP::Samples::RunLinearSolverSample();

    return 0;
}

