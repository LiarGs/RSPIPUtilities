#pragma once
#include "Basic/CloudMask.hpp"
#include "Basic/GeoImage.hpp"
#include "Interface/IImageVisitor.hpp"
#include "Util/General.hpp"
#include <numeric>

namespace RSPIP {

class ImageSaveVisitor : public IImageVisitor {
  public:
    ImageSaveVisitor(const std::string &path, const std::string &name)
        : _Path(path), _Name(name), _Success(false) {}

    void Visit(const Image &image) override {
        _SaveNonGeoImage(image);
    }

    void Visit(const GeoImage &geoImage) override {
        _SaveGeoImage(geoImage);
    }

    void Visit(const CloudMask &cloudMask) override {
        _SaveGeoImage(cloudMask);
    }

    bool IsSuccess() const { return _Success; }

  private:
    void _SaveGeoImage(const GeoImage &geoImage) {
        auto savePath = _Path + _Name;
        GDALAllRegister();
        const char *format = "GTiff";
        GDALDriver *driver = GetGDALDriverManager()->GetDriverByName(format);
        if (!driver) {
            Error("GTiff driver not available.");
            throw std::runtime_error("GTiff driver not available.");
        }

        int imgWidth = geoImage.Width();
        int imgHeight = geoImage.Height();
        int imgBands = geoImage.GetBandCounts();
        int mainBands = geoImage.ImageData.channels();

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
            if (mainBands == 3) {
                cv::cvtColor(geoImage.ImageData, writeImg, cv::COLOR_BGR2RGB);
            } else if (mainBands == 4) {
                cv::cvtColor(geoImage.ImageData, writeImg, cv::COLOR_BGRA2RGBA);
            } else {
                writeImg = geoImage.ImageData;
            }

            std::vector<int> bandMap(mainBands);
            std::iota(bandMap.begin(), bandMap.end(), 1);

            // 使用 Dataset 级别的 RasterIO 一次性写入所有交织波段
            CPLErr err = dataset->RasterIO(
                GF_Write, 0, 0, imgWidth, imgHeight,
                writeImg.data, imgWidth, imgHeight,
                Util::CVTypeToGDALType(geoImage.GetImageType()),
                mainBands, bandMap.data(),
                static_cast<int>(writeImg.elemSize()),   // nPixelSpace
                static_cast<int>(writeImg.step),         // nLineSpace
                static_cast<int>(writeImg.elemSize1())); // nBandSpace

            if (err != CE_None) {
                Error("Error writing main image bands");
                GDALClose(dataset);
                _Success = false;
                return;
            }
        }

        // 写入额外波段
        for (size_t i = 0; i < geoImage.ExtraBandDatas.size(); ++i) {
            int bandIndex = mainBands + 1 + static_cast<int>(i);
            GDALRasterBand *band = dataset->GetRasterBand(bandIndex);
            if (band) {
                band->RasterIO(GF_Write, 0, 0, imgWidth, imgHeight,
                               geoImage.ExtraBandDatas[i].data, imgWidth, imgHeight,
                               Util::CVTypeToGDALType(geoImage.ExtraBandDatas[i].type()), 0, 0);
            }
        }

        GDALClose(dataset);
        Info("GeoImage saved successfully in: {}", savePath);
        _Success = true;
    }

    void _SaveNonGeoImage(const Image &image) {
        const cv::Mat &mergedImg = image.ImageData;
        bool success = cv::imwrite(_Path + _Name, mergedImg);
        if (!success) {
            Error("Failed to write image: {}", _Path);
            throw std::runtime_error("Failed to write image: " + _Path);
        } else {
            Info("Image saved successfully in: {}", _Path);
        }
        _Success = success;
    }

  private:
    std::string _Path;
    std::string _Name;
    bool _Success;
};

bool SaveImage(const Image &image, const std::string &imagePath, const std::string &imageName) {
    ImageSaveVisitor visitor(imagePath, imageName);
    image.Accept(visitor);
    return visitor.IsSuccess();
}
} // namespace RSPIP