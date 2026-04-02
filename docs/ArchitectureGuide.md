# RSPIPUtilities Architecture Guide

相关文档：[DevelopmentRules.md](./DevelopmentRules.md)

## 1. 文档定位

这份文档不是使用手册，而是给后续维护者建立统一源码心智模型的架构讲解。目标是让阅读顺序、模块职责、数据流、公共层边界都尽量明确，避免继续从 `samples/` 或单个算法实现反推整个库的设计。

当前文档以仓库现状为准，不再把已经废弃的旧概念当作有效设计基础，例如：

- `GeoImage`
- `CloudMask`
- image visitor 体系
- `DynamicPatch`

这些概念如果在旧提交、旧笔记或历史讨论中出现，只能当作演化背景，不能作为现在新增代码的依据。

## 2. 推荐阅读顺序

建议按下面顺序理解源码，而不是直接钻进某个具体算法：

1. 聚合头：`include/RSPIP.h`
2. 算法入口：`include/Algorithm.h`
3. IO 入口：`include/IO.h`
4. 通用工具入口：`include/Util.h`
5. 基础数据模型：`include/Basic/Image.h`、`include/Basic/GeoInfo.h`
6. 通用掩膜/区域能力：`include/Basic/MaskSelectionPolicy.h`、`include/Basic/RegionExtraction.h`、`include/Basic/RegionGroup.h`
7. 各算法族基类：`CloudDetectionAlgorithmBase`、`ColorBalanceAlgorithmBase`、`EvaluatorAlgorithmBase`、`PreprocessAlgorithmBase`、`MosaicAlgorithmBase`、`ReconstructAlgorithmBase`
8. 具体算法实现
9. `samples/` 中的示例代码

其中 `samples/` 只用于演示如何调用，不是架构真相源。架构边界、基础类型、模块职责都应以前八步为准。

## 3. 模块分层

### `Basic`

`Basic` 放共享基础数据模型和通用区域/掩膜语义，是全库最容易被污染的一层。

当前这里应该理解为“跨多个算法族复用，而且能够独立解释其含义”的基础能力，例如：

- `Image`：统一图像容器
- `GeoInfo`：可选地理信息与像素/地理坐标转换能力
- `MaskSelectionPolicy`：mask 解释规则
- `RegionExtraction` / `RegionAnalysis`
- `RegionGroup` / `RegionPixel`

`Basic` 不应该承接单个算法的私有缓存、候选结构、调试辅助状态或过渡性抽象。

### `Algorithm`

`Algorithm` 是按算法族组织的业务能力层，对外暴露当前支持的算法，对内允许存在族内 `Detail` 实现细节。

当前公开的主要算法族有：

- `CloudDetection`
- `ColorBalance`
- `Evaluation`
- `Mosaic`
- `Preprocess`
- `Reconstruct`

理解上可以把它分成两层：

- `include/Algorithm/<Family>/`：该算法族的公开算法和基类
- `include/Algorithm/<Family>/Detail` 或 `src/Algorithm/<Family>/...`：族内共享或实现细节

### `IO`

`IO` 负责图像读写和 GeoRaster 持久化，对外统一入口是：

- `RSPIP::IO::ReadImage`
- `RSPIP::IO::SaveImage`

现在 IO 的统一原则是：外部世界无论是普通图片还是 GeoRaster，进库后都统一落到 `Image`；有地理信息时再挂到 `Image.GeoInfo` 上。

### `Util`

`Util` 放通用工具，不承载业务策略，不定义算法特定数据结构。当前包括：

- 调试与日志
- 图像显示
- 通用颜色/灰度转换
- 投影转换
- 少量调度和通用辅助能力

如果一个工具只能服务单个算法家族，通常不应该先想到 `Util`，而应该优先考虑该算法自己的 `Detail` 或 `.cpp` 内部。

### `Math`

`Math` 承担数值求解与线性系统支撑，当前重心在稀疏矩阵与求解器。它提供的是数学能力，不负责图像语义。

### `Interface`

`Interface` 是跨模块稳定接口层，目前最核心的是 `IAlgorithm`。这层应尽量小、尽量稳定，不要把业务细节堆进接口层。

## 4. 核心数据流

当前库的主数据流可以概括为：

`ReadImage -> Image/GeoInfo -> Preprocess -> Mosaic/Reconstruct/Evaluation -> SaveImage`

更具体地说：

1. `IO::ReadImage` 负责把外部文件读成 `Image`
2. 如果源数据有地理信息，则写入 `Image.GeoInfo`
3. `Preprocess` 可以先把多幅图像对齐到统一网格
4. `Mosaic` / `Reconstruct` / `ColorBalance` / `Evaluation` 在统一的 `Image` 模型上工作
5. `IO::SaveImage` 根据 `Image.GeoInfo` 是否存在决定是否写出 GeoRaster 元数据

整个库当前已经不再依赖“普通图像类型”和“地理图像类型”分裂的设计，而是统一围绕单一 `Image` 工作。

## 5. 当前最重要的基础概念

### `Image`

`Image` 是全库统一的图像容器。它包含：

- `cv::Mat ImageData`
- `std::string ImageName`
- `std::vector<double> NoDataValues`
- `std::optional<GeoInfo> GeoInfo`

它的职责是承载图像本体和必要元数据，不负责表达具体算法意图。

### `GeoInfo`

`GeoInfo` 是附着在 `Image` 上的可选地理能力。只有算法确实需要地理信息时，才通过 `Image.GeoInfo` 访问它。

这意味着：

- 没有地理信息的图像仍然是合法 `Image`
- 需要地理信息的算法必须显式校验 `GeoInfo`
- 不能再通过新增平行图像类型解决几何问题

### `MaskSelectionPolicy` / `RegionExtraction` / `RegionGroup`

这套能力的设计目的，是把“mask 怎么解释”与“算法怎么处理选中区域”分开。

- `MaskSelectionPolicy` 负责描述哪些像素算被选中
- `BuildSelectionMask` 负责把原始 mask 图解释成二值选区
- `AnalyzeRegions` / `ExtractConnectedRegions` 负责把选中区域提成可用的连通域表示
- `RegionGroup` / `RegionPixel` 负责表达区域结果

这里的重点是：mask 仍然是 `Image`，而不是独立的“魔法子类”。

## 6. 二值掩膜算法 vs 通用选区工具

当前代码中明确区分两类场景：

### 二值掩膜算法

例如部分 `Mosaic`、`Reconstruct`、`ColorBalance` 算法，它们业务语义上就是“非零表示待处理区域”。

这类算法的规则是：

- 不对外公开 `MaskSelectionPolicy`
- 内部固定使用默认二值策略
- 调用者只传 mask 图，不传解释策略

### 通用选区工具 / Evaluator

例如 `PSNREvaluator`、`RMSEEvaluator`、`SSIMEvaluator`、`BoundaryGradientEvaluator`，它们本质上是在“某个选区上评估”，所以保留外部可配置的 `MaskSelectionPolicy`。

这类能力允许外部指定：

- `NonZeroSelected`
- `ValueSetSelected`

因此它们是“通用选区能力”，而不是“固定二值云掩膜算法”。

## 7. 通用模块与 BGR-only 模块

当前库里不是所有算法都已经真正通用。维护时必须区分“数据模型通用”和“算法实现通用”。

### 通用模块

下面这些能力应按“通用 `Image` 容器”理解：

- `Image` / `GeoInfo`
- `ImageReader` / `ImageWriter`
- `RegionExtraction`
- `GeoCoordinateAlign`
- `PixelThreshold`
- 各类 evaluator 的 mask 选区语义

### 仍明显依赖 BGR 的模块

部分算法实现仍然以 OpenCV BGR 影像为直接操作对象，尤其是：

- `ColorBalance` 家族中的 `MatchStatistics`
- `Mosaic` 家族中的多种拼接实现
- `Reconstruct` 家族中的重建实现
- 一切在热路径里依赖 `Util::BGRToGray()` 的逻辑

对这类模块，维护者应该显式承认其 BGR 假设，而不是把它们误写成“看起来通用、实际上只支持 BGR”的半状态。

## 8. 正反例：什么该进 `Basic`，什么不该

### 适合留在 `Basic` 的例子

- `RegionGroup`
- `RegionPixel`
- `MaskSelectionPolicy`
- `RegionAnalysis`

这些类型都满足几个条件：

- 跨多个算法族复用
- 不依赖某个具体算法步骤
- 单独存在时也能解释自己的含义

### 不应该上提到 `Basic` 的例子

- `GeoCoordinateAlign::LocalGridWindow`
- `GeoCoordinateAlign::WarpPreparation`
- `AdaptiveStripMosaicBase` 中的 `InputBundle`
- `AdaptiveStripMosaicBase` 中的 `CandidatePatch`
- `AdaptiveStripMosaicBase` 中的条带缓存、边界段、局部 patch 像素结构

这些结构的共同特点是：

- 生命周期只属于某个算法或某个算法族
- 语义离不开具体实现流程
- 离开所属算法几乎没有复用价值

因此它们就应该待在算法类内部、`Detail`、或实现文件内部。

## 9. 关于 `samples/`

`samples/ExampleScenarios.cpp` 的定位是示例，不是设计文档，不是公共 API 边界说明，也不是“哪些类型该放哪里”的依据。

对维护者来说应保持以下原则：

- 先读库，再读样例
- 先看基类和聚合头，再看示例调用
- 不允许因为样例里某段代码方便，就反过来改坏公共层边界

## 10. 阅读和维护时的总原则

这套库后续最重要的维护目标不是继续堆更多算法，而是保证已有结构越来越清晰：

- 用单一 `Image` 保持统一数据模型
- 用 `GeoInfo` 提供可选地理能力
- 用 `Basic` 承接稳定、可复用、可独立解释的基础能力
- 用 `Algorithm/<Family>/Detail` 或算法类内部承接局部实现细节
- 不再让样例、历史过渡方案或单个算法需求反向污染整体结构

