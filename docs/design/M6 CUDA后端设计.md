# M6 CUDA 后端设计

## 目标与范围

M6 在不改变 CLI 参数和 27 列 CSV Schema 的前提下，为 ParallelPix 增加可选 CUDA 后端。CUDA 与 Sequential 使用相同的图片、配置、中心裁剪、水印和输出模型，M7 负责完整 wall-clock 计时、PNG 持久化、逐像素验证和结果报告。

首版采用单 GPU、单 CUDA stream、单一融合 Kernel。暂不实现多 stream、双缓冲或传输与计算重叠，以保证正确性和 H2D、Kernel、D2H 三段时间的口径清晰。

## 构建模式

`PARALLELPIX_CUDA` 是值为 `AUTO|ON|OFF` 的 CMake Cache 变量：

- `AUTO`：发现 CUDA 编译器时构建 CUDA 后端，否则生成 CPU-only 工程；
- `ON`：必须发现 CUDA 编译器和 Toolkit，否则配置失败；
- `OFF`：明确禁用 CUDA，不查询或链接 Toolkit。

启用时使用 CUDA C++17、`CUDA::cudart` 和独立的 `parallelpix_cuda` 静态库。默认 `CMAKE_CUDA_ARCHITECTURES=120`，调用方可在配置命令中覆盖。Windows 构建会把 `cudart64_12.dll` 复制到可执行文件目录，避免依赖调用进程是否已刷新安装后的 PATH。

## 公共契约

### 共享批次预检

`prepare_processing_batch()` 返回 `BatchPreparationResult`，统一验证：

- 处理配置和输出内存边界；
- 非空批次；
- 输入图片模型；
- 水印颜色与 Alpha 平面；
- 水印尺寸和边距；
- 每张图片的中心裁剪区域。

Sequential 和 CUDA 都调用该入口，后端不再复制预检规则。后续 M5 可复用同一契约。

### 后端可用性

`IBackendExecutor::availability()` 区分：

- 构建未包含 CUDA：CPU-only 工程不注册 CUDA 执行器；
- 已编译但运行时不可用：无设备、驱动/Runtime 不兼容或初始化失败。

M7 对不可用后端记为 `BackendUnavailable` 并继续运行 CPU 后端。

### 实际批大小

`BackendExecution::effective_cuda_batch_size` 返回本次实验实际采用的 CUDA 批大小。正常运行时等于请求值；设备 grid-z 限制或显存回退会改变该值。M7 使用实际值生成输出目录、CSV 和警告日志，且同一实验的预热和正式重复必须保持一致。

## 数据布局

每个 CUDA 批次将变长 BGR 图片打包到连续输入缓冲，并为每张图片生成描述符：

```text
DeviceImageDescriptor {
  input_offset, width, height, stride,
  crop_x, crop_y, crop_width, crop_height
}
```

输出图片尺寸固定，因此按 `batch × output_width × output_height × 3` 连续排列。水印 BGR 与 Alpha 使用独立连续缓冲。输入、输出、描述符、水印和 Alpha 设备缓冲均按需增长并跨调用复用；M7 的预热调用完成正式测量所需的常规分配。

## Kernel 语义

- block 固定为二维 `16×16`；
- `grid.x/y` 覆盖输出宽高，`grid.z` 选择批次中的图片；
- 每个线程处理一个输出像素的三个 BGR 通道；
- 使用半像素坐标进行双线性采样；
- 双线性结果执行第一次最近整数舍入和 0～255 钳制；
- 亮度乘法执行第二次舍入；
- 最后按水印 Alpha 与全局透明度混合并舍入。

CUDA 输出相对 Sequential 的每通道最大绝对误差允许为 1。当前 RTX 5070 Ti Laptop 实测最大误差为 0。

## 运行与计时

每个批次按以下顺序在同一 stream 中执行：

1. H2D：输入、描述符，以及每次处理调用开始时的水印与 Alpha；
2. Kernel：融合缩放、亮度和水印；
3. D2H：固定尺寸输出。

CUDA Event 分别累计三阶段时间。每个 Event stop 都同步，因此三段不重叠。M7 继续使用 `steady_clock` 包围完整后端调用，写入 `compute_ms`；CUDA Event 时间只写入 `h2d_ms`、`kernel_ms`、`d2h_ms`。

所有 CUDA API 返回值、Kernel 启动和 Event 同步都经过检查。资源析构函数不抛异常。

### 完成后轨迹记录

CUDA 只保留单一的完成后轨迹路径。每批结束时把已处理数量和批平均耗时写入当前尝试的内存缓冲；完整尝试成功后才交给 Benchmark Runner。Runner 在 `compute_ms` 与 `end_to_end_ms` 均停止计时后统一输出轨迹日志，M1 只在整个 Benchmark 完成后绘制静态轨迹。系统不再读取 `PARALLELPIX_TRAJECTORY_MODE`，也不提供实时回调或运行中 Plotly 重绘。

## 显存回退

首次出现 `cudaErrorMemoryAllocation` 时：

1. 放弃该次调用的局部输出、阶段时间和进度样本；
2. 将批大小减半，最小为 1；
3. 从图片 0 开始完整重试一次；
4. 成功时报告回退后的实际批大小；
5. 批大小已为 1 或第二次失败时返回 `BackendFailure`，不生成阶段性能指标。

进度样本先缓存在单次尝试内，只有完整尝试成功后才转发，避免失败尝试与重试产生重复轨迹。若显存错误触发从头重试，失败尝试的样本会随局部状态一起丢弃，最终只记录成功重试的数据。

## 模块边界

```text
include/parallelpix/cuda/processor.hpp
src/cuda/
├── processor/
│   ├── processor.cpp
│   ├── batch_executor.cpp
│   └── batch_packer.cpp
├── runtime/{runtime,resources}.cpp
└── kernels/process_batch.cu
```

- `processor`：公共入口、共享预检、可用性和一次回退策略；
- `batch_executor`：stream、Event、可复用缓冲、分批执行和计时；
- `batch_packer`：主机连续打包和输出模型恢复；
- `runtime`：CUDA 错误、设备发现和 RAII 资源；
- `kernels`：设备描述符和融合 Kernel。

M6 不读取文件、不写 CSV、不编码 PNG，也不计算加速比。

## 已知范围

- 只选择设备 0；
- 不做多 GPU、多 stream 或异步流水线；
- M5 OpenMP 仍未实现，最终课程验收前仍需补齐第二类并行组件；
- 性能不设置固定加速倍数门槛，以 Release 原始 CSV 为准。
