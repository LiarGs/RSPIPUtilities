#include "Basic/Image.h"
#include "Interface/IImageVisitor.h"
#include "Util/SuperDebug.hpp"

namespace RSPIP {

Image::Image() : ImageData(), ImageName("Default.png") {}
Image::Image(const cv::Mat &imageData) : ImageData(imageData), ImageName("Default.png") {}
Image::Image(const cv::Mat &imageData, const std::string &imageName) : ImageData(imageData), ImageName(imageName) {}

void Image::Accept(IImageVisitor &visitor) const {
    visitor.Visit(*this);
}

int Image::Height() const { return ImageData.rows; }
int Image::Width() const { return ImageData.cols; }
int Image::GetBandCounts() const { return ImageData.channels(); }

int Image::GetImageType() const {
    if (ImageData.empty())
        return -1;
    return ImageData.type();
}

bool Image::_IsOutOfBounds(int row, int col) const {
    if (ImageData.empty()) {
        SuperDebug::Error("ImageData is empty.");
        throw std::runtime_error("ImageData is empty.");
        return true;
    } else if (row < 0 || row >= Height() || col < 0 || col >= Width()) {
        SuperDebug::Error("Pixel position out of range: ({}, {})", row, col);
        throw std::runtime_error("Pixel position out of range.");
        return true;
    }
    return false;
}

bool Image::_IsOutOfBounds(int row, int col, int band) const {
    if (_IsOutOfBounds(row, col)) {
        return true;
    } else if (band <= 0 || band > GetBandCounts()) {
        SuperDebug::Error("Band index out of range: {} (valid range: 1-{})", band, GetBandCounts());
        throw std::out_of_range("Band index out of range");
        return true;
    }
    return false;
}

} // namespace RSPIP