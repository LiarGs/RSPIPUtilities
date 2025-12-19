#include "EvaluatorAlgorithmBase.hpp"

namespace RSPIP::Algorithm {
// 基于边界梯度的评估算法
class BoundaryGradientEvaluator : public EvaluatorAlgorithmBase {
  public:
    BoundaryGradientEvaluator(const Image &imageData, const Image &referenceImage, const Image &maskImage)
        : EvaluatorAlgorithmBase(imageData), EvaluateResult(0.0), _ReferenceImage(referenceImage), _Mask(maskImage) {}

    void Execute() override {
        _Evaluate();
    }

    void _Evaluate() {
        if (_ImageData.ImageData.empty() || _ReferenceImage.ImageData.empty() || _Mask.ImageData.empty()) {
            return;
        }

        // 计算 Mask 边界
        auto mask = _Mask.ImageData.clone();
        if (mask.channels() > 1) {
            cv::cvtColor(mask, mask, cv::COLOR_BGR2GRAY);
        }
        cv::boxFilter(mask, mask, -1, cv::Size(3, 3), cv::Point(-1, -1), true, cv::BORDER_DEFAULT);
        cv::Mat boundaryMask = (mask > 0) & (mask < 255);

        // 排除图像边缘
        if (boundaryMask.rows > 2 && boundaryMask.cols > 2) {
            boundaryMask.row(0).setTo(0);
            boundaryMask.row(boundaryMask.rows - 1).setTo(0);
            boundaryMask.col(0).setTo(0);
            boundaryMask.col(boundaryMask.cols - 1).setTo(0);
        } else {
            return;
        }

        // 预处理图像：转灰度并归一化为 double [0, 1]
        auto prepareImage = [](const cv::Mat &src) -> cv::Mat {
            cv::Mat gray, dbl;
            if (src.channels() == 3) {
                cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
            } else {
                gray = src;
            }
            gray.convertTo(dbl, CV_64F, 1.0 / 255.0);
            return dbl;
        };

        cv::Mat image = prepareImage(_ImageData.ImageData);
        cv::Mat referenceImage = prepareImage(_ReferenceImage.ImageData);

        cv::Mat kernel = (cv::Mat_<double>(3, 3) << 1, 1, 1, 1, -8, 1, 1, 1, 1) / 8.0;
        cv::filter2D(image, image, -1, kernel);
        cv::filter2D(referenceImage, referenceImage, -1, kernel);

        EvaluateResult = cv::mean(image - referenceImage, boundaryMask)[0];
    }

  public:
    double EvaluateResult;

  private:
    const Image &_ReferenceImage;
    const Image &_Mask;
};

} // namespace RSPIP::Algorithm
