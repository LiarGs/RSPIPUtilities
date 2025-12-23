#pragma once
#include <memory>
#include <string>

namespace RSPIP {
class Image;
class GeoImage;
class CloudMask;
} // namespace RSPIP

namespace RSPIP::IO {

// 读取普通图片
std::unique_ptr<Image> NormalImageRead(const std::string &imagePath);

// 读取带有地理信息的图片
std::unique_ptr<GeoImage> GeoImageRead(const std::string &imagePath);

// 读取云掩膜
std::unique_ptr<CloudMask> CloudMaskImageRead(const std::string &imagePath);

} // namespace RSPIP