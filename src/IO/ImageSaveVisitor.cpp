#include "IO/ImageSaveVisitor.h"
#include "Basic/CloudMask.h"
#include "Basic/GeoImage.h"
#include "Basic/Image.h"
#include "Util/General.h"
#include "Util/SuperDebug.hpp"
#include <gdal_priv.h>
#include <numeric>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace RSPIP::IO {

ImageSaveVisitor::ImageSaveVisitor(const std::string &path, const std::string &name)
    : _Path(path), _Name(name), _Success(false) {
    if (path.back() != '/') {
        _Path += '/';
    }
}

void ImageSaveVisitor::Visit(const Image &image) {
    _SaveNonGeoImage(image);
}

void ImageSaveVisitor::Visit(const GeoImage &geoImage) {
    _SaveGeoImage(geoImage);
}

bool ImageSaveVisitor::IsSuccess() const {
    return _Success;
}

void ImageSaveVisitor::_SaveGeoImage(const GeoImage &geoImage) {
    auto savePath = _Path + _Name;
    const char *format = "GTiff";
    GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(format);
    if (!driver) {
        Error("GTiff driver not available.");
        throw std::runtime_error("GTiff driver not available.");
    }

    int imgWidth = geoImage.Width();
    int imgHeight = geoImage.Height();
    int imgBands = geoImage.GetBandCounts();

    GDALDataset *dataset =
        driver->Create(savePath.c_str(), imgWidth, imgHeight, imgBands, Util::CVTypeToGDALType(geoImage.GetImageType()), nullptr);
    if (!dataset) {
        Error("Failed to create TIFF file: {}", savePath);
        throw std::runtime_error("Failed to create TIFF file: " + savePath);
    }

    if (!geoImage.Projection.empty()) {
        dataset->SetProjection(geoImage.Projection.c_str());
    }
    if (geoImage.GeoTransform.size() == 6) {
        dataset->SetGeoTransform(const_cast<double *>(geoImage.GeoTransform.data()));
    }

    // 优化：直接写入主图像数据（避免 split 和逐波段写入）
    if (!geoImage.ImageData.empty()) {
        cv::Mat writeImg;
        // OpenCV 是 BGR，GDAL GeoTiff 通常需要 RGB
        if (imgBands == 3) {
            cv::cvtColor(geoImage.ImageData, writeImg, cv::COLOR_BGR2RGB);
        } else if (imgBands == 4) {
            cv::cvtColor(geoImage.ImageData, writeImg, cv::COLOR_BGRA2RGBA);
        } else {
            writeImg = geoImage.ImageData.clone();
            if (imgBands >= 3) {
                std::vector<cv::Mat> chans;
                cv::split(writeImg, chans);
                std::swap(chans[0], chans[2]); // 交换 B 和 R
                cv::merge(chans, writeImg);
            }
        }

        std::vector<int> bandMap(imgBands);
        std::iota(bandMap.begin(), bandMap.end(), 1);

        // 使用 Dataset 级别的 RasterIO 一次性写入所有交织波段
        CPLErr err = dataset->RasterIO(
            GF_Write, 0, 0, imgWidth, imgHeight,
            writeImg.data, imgWidth, imgHeight,
            Util::CVTypeToGDALType(geoImage.GetImageType()),
            imgBands, bandMap.data(),
            static_cast<int>(writeImg.elemSize()),   // nPixelSpace (bytes per pixel)
            static_cast<int>(writeImg.step),         // nLineSpace (bytes per line)
            static_cast<int>(writeImg.elemSize1())); // nBandSpace (bytes per band value)

        if (err != CE_None) {
            Error("Error writing image bands");
            GDALClose(dataset);
            _Success = false;
            return;
        }
    }

    GDALClose(dataset);
    Info("GeoImage saved successfully in: {}", savePath);
    _Success = true;
}

void ImageSaveVisitor::_SaveNonGeoImage(const Image &image) {
    if (image.ImageData.empty()) {
        SuperDebug::Error("SaveImage is Empty!");
    }
    const cv::Mat &mergedImg = image.ImageData;

    bool _Success = cv::imwrite(_Path + _Name, mergedImg);
    if (!_Success) {
        SuperDebug::Error("Failed to write image: {}", _Path + _Name);
    } else {
        SuperDebug::Info("Image saved successfully in: {}", _Path + _Name);
    }
}

bool SaveImage(const Image &image, const std::string &imagePath, const std::string &imageName) {
    ImageSaveVisitor visitor(imagePath, imageName);
    image.Accept(visitor);
    return visitor.IsSuccess();
}

} // namespace RSPIP::IO