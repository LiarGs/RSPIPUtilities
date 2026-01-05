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

bool Image::IsOutOfBounds(int row, int col) const {
    if (ImageData.empty()) {
        SuperDebug::Error("ImageData is empty.");
        return true;
    } else if (row < 0 || row >= Height() || col < 0 || col >= Width()) {
        return true;
    }
    return false;
}

bool Image::IsOutOfBounds(int row, int col, int band) const {
    if (IsOutOfBounds(row, col)) {
        return true;
    } else if (band <= 0 || band > GetBandCounts()) {
        return true;
    }
    return false;
}

} // namespace RSPIP