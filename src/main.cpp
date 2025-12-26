// Samples For Use This Utilities
#include "algorithmTest.hpp"

int main(int argc, char *argv[]) {

    SuperDebug::ScopeTimer mainTimer("Main Program");
    GDALAllRegister();
    // _TestForNormalImage();

    // _TestForGeoImageMosaic();

    // _TestForColorBalance();

    // _TestForEvaluate();

    _TestForReconstruct();

    // _TestForLinearSolver();

    return 0;
}