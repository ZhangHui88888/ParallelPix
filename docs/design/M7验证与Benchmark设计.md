# M7 验证与 Benchmark 设计

## 目标与边界

M7 将 M2 的实验计划、M3 图片 I/O 和 M4～M6 处理后端串成真实 Benchmark。它负责后端生命周期、预热、计时、输出验证、统计与 CSV 报告，不实现像素算法，也不修改 M1/M2 的命令或 27 列 CSV 契约。

当前 M7 Core 注册 Sequential 执行器。OpenMP、CUDA 未实现时，对应实验计为跳过；只要 Sequential 成功，CLI 返回部分成功并保留有效 CSV。M5/M6 后续只需实现并注册 `IBackendExecutor`。

## 模块与构建边界

```text
parallelpix_m2
  └─ CLI、BenchmarkPlan、IBenchmarkPipeline、WorkflowSummary

parallelpix_benchmark
  ├─ statistics / validation / reporting
  ├─ runner / SequentialExecutor
  ├─ BenchmarkPipeline 工厂
  └─ 依赖 parallelpix_io、parallelpix_sequential
```

`parallelpix_m2` 不再包含具体 Pipeline，避免 M2 与 M7 循环依赖。可执行程序链接 `parallelpix_benchmark`，后者传递 M2 控制层依赖。

## 公共契约

- `SummaryStatistics`：中位数、最小值、最大值和总体标准差。
- `ValidationResult`：验证结论、可选最大像素误差、结构错误位置与消息。
- `BackendExecution`：后端处理结果和可选 CUDA 分阶段时间。
- `IBackendExecutor`：报告后端类型，并以图片、水印、公共配置和实验项执行一次处理。
- `BenchmarkRecord`：严格对应 M1 要求的 27 列；不适用值使用空字段。

M7 通过 `run_benchmark_plan()` 接受执行器集合，因此测试和后续 M5/M6 均不需要改动 Controller。

## 执行流程

1. 校验 CSV 目标、图片输出目录、输入目录和水印。
2. 加载最大图片规模，冻结本次运行使用的有序源文件列表。
3. 为本次 CLI 调用生成唯一 `run_id` 和统一 UTC 记录时间。
4. 按 `BenchmarkPlan` 顺序处理实验：
   - 后端未注册时标记 `skipped`；
   - 非 Sequential 实验缺少同规模基准时标记 `skipped`；
   - 使用预解码批次执行 2 次计算预热；
   - 每次正式测量重新扫描、解码并检查源文件列表未变化；
   - 后端计算后将 PNG 写入运行专属目录；
   - 在计时区间外重新解码 PNG 并验证。
5. 聚合所有完整实验，将有效和验证失败的记录一次性写入 CSV。
6. 返回与计划项数量严格一致的成功、失败和跳过统计。

单项失败不会阻止独立配置继续运行。全局输入或输出预检失败时，所有计划项计为失败且不创建 CSV。

## 计时与统计口径

- 预热仅调用计算后端，不计时、不写 PNG。
- `compute_ms` 使用 `steady_clock` 包围单次后端调用。
- `end_to_end_ms` 包含目录扫描、图片解码、计算和 PNG 编码，不包含验证与 CSV 写入。
- 正式样本至少 5 次；偶数样本中位数取中间两项平均值，标准差除以样本总数。
- 吞吐量使用计算时间中位数：

```text
images_per_second = image_count / compute_seconds
megapixels_per_second =
  image_count × output_width × output_height / 1,000,000 / compute_seconds
```

- Sequential 验证通过时 `speedup=1`。
- OpenMP/CUDA 只在验证通过且同规模 Sequential 有效时计算加速比。
- 并行效率只适用于 OpenMP；CUDA 的 H2D、Kernel、D2H 分别保存中位数。

## 输出与验证

图片输出目录：

```text
<output>/<run_id>/<backend>/
  images-<count>[-threads-<n>|-batch-<n>]/
    000001-<source-stem>.png
```

每个正式重复覆盖同一运行专属文件，最终保留最后一次输出，不删除用户已有目录或文件。

- Sequential：内存结果与重新解码的自身 PNG 完全一致。
- OpenMP：持久化 PNG 与同规模 Sequential PNG 完全一致。
- CUDA：每通道最大绝对误差不超过 1。
- 尺寸、通道、步长或批次数不一致属于结构失败，不伪造最大像素误差。
- 验证失败行仍写入 CSV，但 `validation_passed=false`，`speedup` 和 `parallel_efficiency` 为空。

## CSV 一致性

- 一次 CLI 调用的所有行共享 `run_id` 和 `recorded_at_utc`。
- `input_resolution` 在所有图片一致时写 `宽x高`，否则写 `mixed`。
- Overwrite 通过同目录临时文件原子替换目标。
- Append 先验证既有表头完全一致，再把旧内容和新行写入临时文件并原子替换。
- 写入失败时保留原文件；本地单进程运行，不提供并发写锁。

## 错误与退出状态

| 场景 | Workflow 行为 | CLI 结果 |
|---|---|---|
| 输入、水印或图片数量无效 | 全部失败，`Input` | 65 |
| CSV 或图片输出失败 | `Output`，不提供成功 CSV | 73 |
| Sequential 处理或验证失败 | 当前项失败，依赖项跳过 | 70 或部分成功 |
| OpenMP/CUDA 未注册 | 对应项跳过，`BackendUnavailable` | 有 Sequential 时为 2 |
| 全部项成功 | 提供 CSV | 0 |

