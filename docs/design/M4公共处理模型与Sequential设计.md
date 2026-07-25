# M4 公共处理模型与 Sequential 设计

## 目标与边界

M4 定义三类计算后端共用的图片处理配置、几何与数值语义，并提供不创建工作线程的 C++17 Sequential 正确性基线。M4 消费 M3 的 `Image`、`Watermark`，返回内存中的标准化图片批次。

M4 不负责扫描、解码、输出命名、PNG 写出、预热、重复测量、统计、CSV 或 CLI Pipeline 编排。M7 完成上述 Benchmark 能力后，才替换当前 `ControllerOnlyPipeline`。

## 公共契约

`ProcessingConfig` 默认值：

| 配置 | 默认值 | 规则 |
|------|--------|------|
| 输出宽高 | 1024×1024 | 必须大于 0，且像素缓冲大小不能溢出 |
| 亮度系数 | 1.10 | 必须是有限正数 |
| 全局水印透明度 | 0.35 | 必须是 `[0,1]` 内的有限数 |
| 水印右/下边距 | 32px | 水印保持原尺寸并完整放入输出 |

Sequential 公开入口为无状态函数：

```text
process_batch(images, watermark, config) -> BatchProcessingResult
```

结果要么包含与输入数量和顺序一致的完整 `Image` 批次，要么只包含结构化问题；不返回部分图片。输出保留每张输入的 `source_path`，调用不修改输入图片和水印。

## 固定像素语义

1. 计算与输出宽高比一致的最大中心裁剪区域；差值为奇数时，左/上偏移使用整数向下取整。
2. 使用半像素坐标映射执行双线性缩放：

   ```text
   source = crop_origin + (output_coordinate + 0.5) * scale - 0.5
   ```

   采样坐标钳制在裁剪区域内，每个输出通道读取四个相邻 BGR 值。
3. 双线性结果先按最近整数舍入并钳制到 `[0,255]`，再乘亮度系数并执行同样舍入。
4. 水印保持原始尺寸，放在距右边和下边各 32px 的位置。每个像素的有效 Alpha 为：

   ```text
   effective_alpha = watermark_alpha / 255 * global_opacity
   output = round(base * (1 - effective_alpha) + watermark * effective_alpha)
   ```

公共处理模块是 M5/M6 的语义基准。OpenMP 必须复用相同 CPU 公共函数以获得逐像素一致结果；CUDA 可按同一公式实现，并按项目要求接受每通道最大误差 1。

## Sequential 实现

- 按批次顺序逐张处理，内部循环顺序为输出行、输出列、BGR 通道。
- 中心裁剪只保存坐标，不创建裁剪图。
- 每张图片只分配最终输出缓冲；缩放和亮度在主像素循环完成，水印在覆盖区域进行第二次原地混合。
- `parallelpix_sequential` 不链接 OpenMP 或 OpenCV，不创建工作线程；M3 的 OpenCV 解码和编码不属于核心计算区间。

## 错误模型

| 错误码 | 条件 |
|--------|------|
| `InvalidConfig` | 尺寸、亮度、透明度或内存大小非法 |
| `EmptyBatch` | 输入批次为空 |
| `InvalidImage` | 任一输入不满足紧密三通道 BGR 模型 |
| `InvalidWatermark` | 水印颜色图或 Alpha 平面无效 |
| `WatermarkDoesNotFit` | 原尺寸水印加右/下边距超出输出 |

图片错误携带输入索引和 `source_path`。预期校验失败通过结果对象返回；内存分配等非预期运行时异常不伪装成业务错误。

## 集成状态

M4 已通过自动化测试证明 `M3 扫描/解码 → M4 Sequential → M3 PNG 写出/回读` 链路成立。M7 现已将该链路接入公开 `parallelpix benchmark`，并在不改变 M4 接口的前提下负责预热、计时、验证、统计和 CSV。
