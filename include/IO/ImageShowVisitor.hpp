#pragma once
#include "Basic/CloudMask.hpp"
#include "Basic/GeoImage.hpp"
#include "Interface/IImageVisitor.hpp"

namespace RSPIP {

class ImageShowVisitor : public IImageVisitor {
  public:
    explicit ImageShowVisitor(int band = 0) : _Band(band) {}

    void Visit(const Image &image) override {
        _Show(image);
    }

    void Visit(const GeoImage &image) override {
        _Show(image);
    }

    void Visit(const CloudMask &image) override {
        _Show(image);
    }

  private:
    void _Show(const Image &image) {
        if (_Band > 0) {
            if (_Band > image.GetBandCounts()) {
                Error("Invalid band number: {}", _Band);
                throw std::runtime_error("Invalid band number: " + std::to_string(_Band));
            }
            std::vector<cv::Mat> bandDatas = {};
            cv::split(image.ImageData, bandDatas);
            const auto &bandData = bandDatas[_Band - 1];

            if (bandData.empty()) {
                Error("Image band {} is empty", _Band);
                throw std::runtime_error("Image band " + std::to_string(_Band) + " is empty.");
            }

            auto windowName = image.ImageName + " Band " + std::to_string(_Band);
            cv::namedWindow(windowName, cv::WINDOW_NORMAL);
            cv::imshow(windowName, bandData);
            cv::waitKey(0);
        } else {
            if (image.ImageData.empty()) {
                Error("Image pointer is null.");
                throw std::runtime_error("Image pointer is null.");
            }

            cv::namedWindow(image.ImageName, cv::WINDOW_NORMAL);
            cv::imshow(image.ImageName, image.ImageData);
            cv::waitKey(0);
        }
    }

  private:
    int _Band;
};

inline void ShowImage(const Image &image, int band) {
    ImageShowVisitor visitor(band);
    image.Accept(visitor);
}

inline void ShowImage(const Image &image) {
    ImageShowVisitor visitor(0);
    image.Accept(visitor);
}

} // namespace RSPIP