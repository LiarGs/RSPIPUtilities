#include "Basic/Image.h"
#include "Util/SuperDebug.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <type_traits>

namespace RSPIP {

namespace {

bool _HasExplicitNoDataValue(const std::vector<double> &noDataValues) {
    return std::ranges::any_of(noDataValues, [](double value) {
        return !std::isnan(value);
    });
}

template <typename T>
bool _PixelEqualsNoData(const cv::Mat &imageData, int row, int col, const std::vector<double> &noDataValues) {
    const int channels = imageData.channels();
    const T *pixelPtr = imageData.ptr<T>(row) + col * channels;
    bool hasComparableChannel = false;
    for (int channelIndex = 0; channelIndex < channels; ++channelIndex) {
        const double expectedValue = noDataValues.size() == 1 ? noDataValues.front()
                                                              : noDataValues[static_cast<size_t>(std::min(channelIndex, static_cast<int>(noDataValues.size()) - 1))];
        if (std::isnan(expectedValue)) {
            continue;
        }

        hasComparableChannel = true;
        if constexpr (std::is_floating_point_v<T>) {
            if (std::abs(static_cast<double>(pixelPtr[channelIndex]) - expectedValue) > 1e-6) {
                return false;
            }
        } else if (static_cast<double>(pixelPtr[channelIndex]) != expectedValue) {
            return false;
        }
    }
    return hasComparableChannel;
}

} // namespace

Image::Image() : ImageData(), ImageName("Default.png"), NoDataValues(), GeoInfo(std::nullopt) {}
Image::Image(const cv::Mat &imageData) : ImageData(imageData), ImageName("Default.png"), NoDataValues(), GeoInfo(std::nullopt) {}
Image::Image(const cv::Mat &imageData, const std::string &imageName)
    : ImageData(imageData), ImageName(imageName), NoDataValues(), GeoInfo(std::nullopt) {}

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

bool Image::HasNoData() const {
    return _HasExplicitNoDataValue(NoDataValues);
}

bool Image::IsNoDataPixel(int row, int col) const {
    if (IsOutOfBounds(row, col) || !HasNoData()) {
        return false;
    }

    switch (ImageData.depth()) {
    case CV_8U:
        return _PixelEqualsNoData<unsigned char>(ImageData, row, col, NoDataValues);
    case CV_8S:
        return _PixelEqualsNoData<signed char>(ImageData, row, col, NoDataValues);
    case CV_16U:
        return _PixelEqualsNoData<unsigned short>(ImageData, row, col, NoDataValues);
    case CV_16S:
        return _PixelEqualsNoData<short>(ImageData, row, col, NoDataValues);
    case CV_32S:
        return _PixelEqualsNoData<int>(ImageData, row, col, NoDataValues);
    case CV_32F:
        return _PixelEqualsNoData<float>(ImageData, row, col, NoDataValues);
    case CV_64F:
        return _PixelEqualsNoData<double>(ImageData, row, col, NoDataValues);
    default:
        SuperDebug::Error("Unsupported image depth for NoData comparison: {}", ImageData.depth());
        return false;
    }
}

cv::Scalar Image::GetFillScalarForOpenCV() const {
    if (NoDataValues.empty()) {
        return cv::Scalar();
    }

    const auto resolveChannelValue = [&](size_t index, double fallback) {
        if (index >= NoDataValues.size() || std::isnan(NoDataValues[index])) {
            return fallback;
        }
        return NoDataValues[index];
    };

    const double v0 = resolveChannelValue(0, 0.0);
    const double v1 = resolveChannelValue(1, v0);
    const double v2 = resolveChannelValue(2, v1);
    const double v3 = resolveChannelValue(3, v2);
    return cv::Scalar(v0, v1, v2, v3);
}

bool Image::IsBgrColorImage() const {
    return ImageData.type() == CV_8UC3;
}

} // namespace RSPIP
