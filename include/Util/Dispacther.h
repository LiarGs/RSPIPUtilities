#pragma once
#include "Util/SuperDebug.hpp"
#include <functional>
#include <opencv2/core/hal/interface.h>

namespace RSPIP::Util {

/**
 * @brief 运行时类型分发器
 * 根据 OpenCV 的 depth (如 CV_8U, CV_32F) 调用对应的泛型 Lambda
 *
 * 使用示例:
 * Util::Dispatch(image.depth(), [&]<typename T>() {
 * // 这里 T 会被替换为 uchar, float 等
 * // 调用你的模板函数:
 * MyAlgorithm<T>(...);
 * });
 */
template <typename Functor>
void DispatchDepth(int depth, Functor &&func) {
    switch (depth) {
    case CV_8U:
        func.template operator()<uchar>();
        break;
    case CV_8S:
        func.template operator()<schar>();
        break;
    case CV_16U:
        func.template operator()<ushort>();
        break;
    case CV_16S:
        func.template operator()<short>();
        break;
    case CV_32S:
        func.template operator()<int>();
        break;
    case CV_32F:
        func.template operator()<float>();
        break;
    case CV_64F:
        func.template operator()<double>();
        break;
    default:
        SuperDebug::Error("Dispatcher: Unsupported depth type: {}", depth);
        throw std::runtime_error("Unsupported depth type");
    }
}

template <typename Functor>
void DispatchType(int type, Functor &&func) {
    switch (type) {
    case CV_8UC3:
        func.template operator()<cv::Vec3b>();
        break;
    default:
        SuperDebug::Error("Dispatcher: Unsupported depth type: {}", type);
        throw std::runtime_error("Unsupported depth type");
    }
}

} // namespace RSPIP::Util