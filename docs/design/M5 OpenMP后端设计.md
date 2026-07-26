# M5 OpenMP 后端设计

## 目标与边界

M5 使用 OpenMP 实现可配置线程数的多核 CPU 后端，在不改变 M4 公共像素语义的前提下，加速批量图片标准化处理。M5 消费 M3 的 `Image`、`Watermark` 和 M4 的 `ProcessingConfig`，返回与 Sequential 相同结构的内存图片批次。

M5 不扫描或解码图片，不写 PNG、CSV，不计算加速比，也不决定实验顺序。M7 通过 `IBackendExecutor` 调用 M5，并继续负责预热、计时、验证、统计和报告。

## 公共入口

```text
openmp::process_batch(
  images,
  watermark,
  config,
  thread_count,
  progress
) -> BatchProcessingResult
```

- `thread_count` 必须是可表示为正 `int` 的值；
- 输出数量、顺序和 `source_path` 与输入一致；
- 任一输入、配置或水印无效时整批失败，不返回部分图片；
- 输入图片、水印和公共配置均为只读。

## 并行调度

M5 首版不使用嵌套并行，根据批次规模选择一种调度方式：

| 条件 | 策略 |
|------|------|
| `image_count >= thread_count` 或单线程 | 按图片并行 |
| `image_count < thread_count` | 逐张处理，并在单张图片内部按输出行并行 |

按图片并行使用 `schedule(dynamic, 1)`，每个线程独占一张图片的最终输出缓冲，适配不同输入分辨率造成的计算差异。按行并行使用 `schedule(static)`，每个线程写入互不重叠的输出行；缩放/亮度阶段和水印阶段分别建立并行区域，不发生嵌套。

所有最终输出缓冲在进入 OpenMP 并行区域前分配，工作线程只执行不抛异常的像素计算。

## 像素一致性

OpenMP 后端直接调用 M4 公共函数：

- `compute_center_crop`
- `sample_bilinear`
- `apply_brightness`
- `blend_watermark_channel`

处理顺序保持为中心裁剪、双线性缩放、亮度调整、水印混合。由于每个输出像素只由一个线程写入且不进行浮点归约，OpenMP 输出应与 Sequential 逐像素完全一致。

## 进度与错误

- 每完成 10 张图片或到达批次末尾时产生一次进度样本；
- 图片级并行下通过互斥保护已完成数量和进度回调；
- 进度回调是观察能力，回调异常不会使已完成的图片处理失败；
- M5 返回 M4 共用的 `ProcessingIssue` 错误模型；
- M7 将 M5 错误映射为日志、实验失败和 CLI 退出状态。

## M7 接入

`OpenMpExecutor` 从 `ExperimentSpec.thread_count` 取得线程数，调用 M5 后端，并通过既有 `ProgressSink` 输出轨迹。Pipeline 默认注册 Sequential 和 OpenMP 执行器；CUDA 未注册时仍按既有规则跳过并返回部分成功。

CSV 契约不变。OpenMP 有效记录包含 `thread_count`、`speedup` 和 `parallel_efficiency`，CUDA 专属字段保持为空。

## 验收标准

- 1、2、4、8 线程配置可被真实 Pipeline 执行；
- 图片级和行级两种调度均与 Sequential 逐像素一致；
- 输出顺序、来源路径和输入不变性保持；
- M7 为 OpenMP 生成通过验证的 CSV 行；
- 混合请求执行 Sequential/OpenMP，CUDA 不可用时只跳过 CUDA；
- 不改变 CLI 和 27 列 CSV 契约。
