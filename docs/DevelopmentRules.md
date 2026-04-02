# RSPIPUtilities Development Rules

相关文档：[ArchitectureGuide.md](./ArchitectureGuide.md)

## 1. 文档定位

这份文档是维护规则，不是风格建议。它的目标是降低后续重构和新增算法时的结构漂移，尤其是避免：

- 新增算法私有结构污染 `Basic`
- 局部需求反向污染公共聚合头
- 样例代码变成事实上的架构规范
- 相同校验、失败语义、mask 解释逻辑在各处重复发明

## 2. 分层放置规则

### 总原则

如果一个类型、缓存、辅助函数、临时结构、候选结构只服务单个算法或单个算法族，就不要放进 `Basic`。

优先级固定如下：

1. 算法类内部
2. 同目录 `Detail`
3. `.cpp` 匿名命名空间
4. 只有在跨多个算法族复用时，才考虑提升到 `Basic`

### 决策表

| 场景 | 应放位置 | 不应放位置 |
| --- | --- | --- |
| 全库共享的数据容器 | `Basic` | 某个算法目录 |
| 多个算法族复用的通用掩膜/区域能力 | `Basic` | 单个算法 `.cpp` |
| 单个算法的候选 patch、局部缓存、边界结构 | 算法类内部 / `Detail` | `Basic` / `Util` |
| 单个算法族共享但不打算公开的逻辑 | `Algorithm/<Family>/Detail` | 聚合头 / `Basic` |
| 仅在一个 `.cpp` 内使用的帮助函数 | `.cpp` 匿名命名空间 | 头文件 |
| 通用数学能力 | `Math` | `Util` / `Basic` |
| 通用日志/显示/投影转换工具 | `Util` | `Basic` |

### 必须遵守的核心约束

`Basic` 只允许放“稳定、跨多个算法族复用、能独立解释的基础类型或通用能力”。  
如果不是通用数据结构，就不要污染 `Basic`，优先收束在自己的算法类文件、族内 `Detail` 或 `.cpp` 内部。

## 3. 新增算法时的规则

### 放置规则

- 公开算法才进入 `include/Algorithm/<Family>/`
- 族内共享但不打算成为公共 API 的逻辑放 `Algorithm/<Family>/Detail`
- 只服务单个算法的结构和帮助函数，优先放在该算法头/源文件内部
- 过渡性算法、实验性算法、一次性验证算法，不进入公共聚合头

### 聚合头规则

- 只有稳定对外能力才能进入 `include/Algorithm.h` 及各家族聚合头
- 如果某个算法还处在实验或频繁变动阶段，不要急着放入聚合头
- 删除过渡性算法时，要同步清理聚合头和相关注释，避免它继续被当作公共能力

### 基类与输入输出规则

- 所有算法统一以 `Image` 作为输入/输出容器
- 需要地理信息时，通过 `Image.GeoInfo` 校验
- 不再通过新增平行图像类型表达地理图像、mask 图像或其他特化图像
- 算法输入按值持有，避免生命周期不一致

## 4. 掩膜、区域与地理信息规则

### mask 规则

- mask 本质上仍然是 `Image`
- mask 的语义通过 `MaskSelectionPolicy` 决定，不通过子类决定
- 只有 evaluator 和通用选区工具保留对外可配置 `MaskSelectionPolicy`
- 明确依赖二值 mask 的算法，不对外公开 `MaskSelectionPolicy`

### region 规则

- 需要连通域时，优先复用 `AnalyzeRegions` / `BuildSelectionMask` / `ExtractConnectedRegions`
- 不要在各算法里重复实现一套自己的二值解释和连通域提取
- 新增区域结构前，先判断现有 `RegionGroup` / `RegionPixel` 是否已经足够表达

### Geo 规则

- 地理能力统一从 `Image.GeoInfo` 进入
- 需要地理信息的算法必须显式校验 `GeoInfo`
- 不能为“方便调用”重新引入 `GeoImage` 一类平行数据模型

## 5. 失败行为与校验规则

### 图像算法

产出 `Image` 的算法在失败时必须统一清空：

- `ImageData`
- `ImageName`
- `NoDataValues`
- `GeoInfo`

优先复用 `Algorithm::Detail::ResetImageResult`，不要自行发明不同的失败状态。

### 数值评估算法

产出数值的 evaluator 在失败时统一返回 `NaN`，而不是 `0.0` 或其他模糊值。优先复用 `Algorithm::Detail::ResetEvaluationResult`。

### 校验逻辑

以下常见校验应优先复用 `Algorithm::Detail::AlgorithmValidation`：

- 非空图像校验
- 尺寸一致性校验
- `GeoInfo` 可用性校验
- BGR-only 输入校验

不要每个算法各写一套类似但不一致的错误处理。

## 6. 命名与职责规则

### 命名

- 名称要表达真实职责，不沿用历史残留概念
- 已废弃的 visitor、`GeoImage`、`CloudMask`、`DynamicPatch` 等命名不应重新复活
- 如果一个名字会让维护者误以为它是基础模型或公共能力，但实际上只是局部实现，就应该降级到更局部的位置

### `Util` 规则

- `Util` 只放真正通用的工具
- 不能因为“几个算法都想顺手用”就把业务逻辑扔进 `Util`
- 业务意义强的辅助逻辑，优先留在所属算法族

### `samples` 规则

- `samples/` 是调用示例，不是架构规范
- 不允许从样例反推公共 API 设计
- 样例中的本地路径、调试流程、一次性参数设置，都不应倒逼核心库结构变化

## 7. 正反例

### 正例

- `Image`、`GeoInfo` 放在 `Basic`
- `MaskSelectionPolicy`、`RegionExtraction` 放在 `Basic`
- `GeoCoordinateAlign::LocalGridWindow` 放在算法类内部
- `AdaptiveStripMosaicBase` 的候选 patch、边界段、条带缓存收束在算法内部

### 反例

- 因为某个算法实现里要反复使用某个结构，就把它上提到 `Basic`
- 因为某个工具函数“也许以后还能用”，就先放进 `Util`
- 因为样例里调用方便，就把实验性算法放进聚合头
- 因为历史上存在过 `GeoImage` 或 `CloudMask`，就重新引入平行类型

## 8. 新增代码前检查清单

在写新代码前，至少回答下面这些问题：

- 这是基础类型，还是算法私有类型？
- 这项能力会被几个算法族复用？
- 它是否能脱离具体算法独立解释？
- 是否真的需要进入聚合头？
- 是否会引入新的 `Basic` 污染？
- 是否已经能用现有的 `Image`、`GeoInfo`、`RegionExtraction`、`AlgorithmValidation` 表达？
- 是否只是一个单个 `.cpp` 里的临时帮助函数？
- 如果以后删除这个算法，这个结构是否还值得单独存在？

只要以上问题里有多个答案指向“局部使用”，就不应把代码上提到公共层。

## 9. 维护目标

后续维护的目标不是把所有逻辑都推平到公共层，而是让层次越来越清楚：

- `Basic` 更稳定，而不是更臃肿
- `Algorithm` 更清晰，而不是更分散
- `IO` 只做读写，不背业务语义
- `Util` 只做工具，不背算法策略
- `samples` 只做演示，不指导架构

如果新增代码破坏了这些边界，应优先修改新增代码，而不是继续放宽规则。
