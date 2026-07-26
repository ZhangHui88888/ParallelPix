# M8 CPU-GPU 混合后端设计

## 目标与定位

M8 是 M5 OpenMP 与 M6 CUDA 完成后的异构并行扩展实验。它把同一批图片拆成互不重叠的 CPU 子集和 GPU 子集，让 OpenMP 与 CUDA 在同一段 wall-clock 时间内并发执行，再按原始输入顺序合并结果。

M8 的研究问题是：在目标机器上，利用 CPU 闲置算力能否缩短纯 CUDA 的端到端计算时间，以及最佳 CPU/GPU 工作比例如何随图片数量、CPU 线程数和 CUDA 批大小变化。

M8 不替代 M5 或 M6，也不作为课程“两类技术组成”的替代证明。课程基础验收仍由独立 OpenMP 与 CUDA 后端完成；混合模式不设置固定加速门槛，允许实验结论为“没有快于纯 CUDA”。

## 依赖与启用条件

```text
M3 图片 I/O
  ├→ M4 Sequential
  ├→ M5 OpenMP ─┐
  └→ M6 CUDA ───┴→ M8 Hybrid → M7 验证与 Benchmark → M1 仪表板
```

- 必须先完成 M5、M6 和 M7 的独立闭环；
- Hybrid 仅在 OpenMP 已编译且 CUDA Runtime 可用时可用；
- 任一组件不可用时，M8 标记为 `BackendUnavailable`，Sequential、OpenMP 和 CUDA 独立配置继续运行；
- 不自动把失败的 Hybrid 配置伪装成纯 OpenMP 或纯 CUDA 结果。

## 配置契约

M8 复用现有线程数和 CUDA 批大小，并新增 CPU 工作比例：

```text
backend = hybrid
thread_count = OpenMP 线程数
cuda_batch_size = CUDA 请求批大小
hybrid_cpu_share = 分配给 CPU 的图片百分比
```

计划新增 `--hybrid-cpu-shares`，接受逗号分隔的 1～99 整数。`0` 和 `100` 不进入 Hybrid 矩阵，分别由纯 CUDA 和纯 OpenMP 配置表达，避免重复实验。

M8 是新的接口版本，允许在既有27列 CSV 后追加第28列 `hybrid_cpu_share`。M1 必须向后兼容历史27列文件：旧文件加载时该列补为空值，新文件不得重解释或覆盖已有字段。M7 在首次向旧27列文件 Append M8 结果时，必须通过临时文件把旧行原子升级为“第28列为空”的新 Schema，再追加新行；表头或旧行不合法时停止写入并保留原文件。CPU 分配比例不得编码进 `backend` 字符串，也不得占用 `parallel_efficiency` 等已有指标。

## 静态任务划分

首版采用确定性的静态加权分配：

1. 根据 `image_count × hybrid_cpu_share` 计算 CPU 图片数量，并在图片数至少为2时钳制到 `1～image_count-1`；
2. 使用均匀交错方式从稳定输入序列中选取 CPU 索引，避免连续高分辨率图片全部落到同一设备；
3. 其余索引交给 CUDA；
4. 两个子集都保存原始索引；
5. 完成后按原始索引写回最终输出数组。

CPU 与 GPU 处理不同图片，不重复计算同一张图片。首版不做运行中抢占、工作窃取或自适应迁移，因为 CUDA 批次提交后不能低成本转移到 CPU，动态调度也会使计时和复现实验更复杂。

Hybrid 至少需要2张图片；图片数为1时不能让两个分支都获得任务，该配置在计划阶段判为非法。纯 OpenMP 或纯 CUDA 仍可测试单张图片。

## 并发执行

Hybrid Executor 在完成公共配置、图片、水印和裁剪预检后启动两个并发分支：

- CPU 分支调用 M5 OpenMP Processor，使用配置的线程数；
- GPU 分支调用 M6 CUDA Processor，使用配置的批大小和既有显存回退策略。

编排线程不参与像素计算，避免与 OpenMP 工作线程争抢任务。两个分支只读共享水印和配置，各自拥有输入索引、输出容器和错误状态；合并前不写同一输出位置。

任一分支失败时，整个 Hybrid 配置失败并丢弃局部输出、验证结果和性能指标。CUDA 批大小回退成功时记录实际批大小；再次失败则按 Hybrid 失败处理。

## 进度与完成后轨迹

M8 使用线程安全的内存进度协调器合并 CPU 与 GPU 样本，两个工作分支在被测区间内不得写控制台或触发页面重绘。Benchmark 计时结束后，M1 静态展示三类轨迹：

- `Hybrid CPU`：OpenMP 子集的批次或区间耗时；
- `Hybrid GPU`：CUDA 子集的批次耗时；
- `Hybrid overall`：两个分支合并后的累计完成图片数。

总体完成数使用原子计数或互斥保护，范围为 `0～image_count`。M1 保留完整视图，并增加“并行后端放大视图”，排除 Sequential 后自动缩放 Y 轴，使 OpenMP、CUDA 与 Hybrid 的小时间差可见。

正式性能结论仍以 CSV 的 `compute_ms`、吞吐量和加速比为准；完成后轨迹仅用于复查负载不均、停顿和异常。

## 计时与指标

M7 使用单个 `steady_clock` 包围整个 Hybrid 执行，`compute_ms` 表示：

```text
任务划分 + CPU/GPU 并发执行 + 输出合并
```

由于两个分支并发，Hybrid 总时间不是 CPU 时间与 GPU 时间之和，而近似为较慢分支时间加编排开销。CUDA 的 `h2d_ms`、`kernel_ms` 和 `d2h_ms` 只描述 GPU 子集。`parallel_efficiency` 对 Hybrid 留空，避免用单一线程数解释异构资源效率。

Hybrid 的加速比仍以相同图片数量的 Sequential `compute_ms` 为基线：

```text
Hybrid Speedup = Sequential compute_ms / Hybrid compute_ms
```

M8 性能报告必须同时对照纯 OpenMP 与纯 CUDA，不能只报告相对 Sequential 的加速。

## 正确性

- 合并输出的数量、顺序、来源路径和尺寸必须与输入一致；
- CPU 子集在 M8/M5 单元测试中保持与 Sequential 完全一致；
- GPU 子集允许每通道最大绝对误差不超过 1；
- 合并后由 M7 对整批输出执行 Sequential 对照验证，整批最大误差不超过 1；
- 任一图片丢失、重复、错序或超出误差时，Hybrid 配置无效且不得生成加速结论。

## 首版范围外

- 单张图片内部由 CPU 与 GPU 共同处理；
- 多 GPU、多 CUDA stream、双缓冲和传输/计算重叠；
- 运行中工作窃取、在线自动调参或跨运行机器学习调度；
- 同时执行多个 Hybrid 配置；
- 固定要求 Hybrid 必须快于纯 CUDA。

## 验收条件

- Hybrid 的 CPU 与 GPU 分支确实在时间上重叠，而非顺序执行；
- 1～99 的 CPU 比例均能生成确定、无重复的任务划分；
- 输出顺序和逐像素验证通过；
- 组件不可用、任一分支失败和 CUDA 回退均有明确行为；
- CSV 能区分线程数、实际 CUDA 批大小和 CPU 比例，旧27列结果仍可读取；
- 完成后静态轨迹能区分 Hybrid CPU、Hybrid GPU 和总体进度；
- Release 下完成至少三种图片规模的粗调与细调实验；
- 无论 Hybrid 是否快于纯 CUDA，都记录真实结果和原因分析。
