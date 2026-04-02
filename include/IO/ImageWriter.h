#pragma once
#include "Basic/Image.h"
#include <string>

namespace RSPIP::IO {

bool SaveImage(const Image &image, const std::string &imagePath, const std::string &imageName);

} // namespace RSPIP::IO
