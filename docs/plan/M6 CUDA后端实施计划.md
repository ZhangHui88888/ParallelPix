# M6 CUDA 后端实施计划

## 实施目标

在 M3、M4、M7 已完成的基础上直接实施 M6，不依赖 M5/OpenMP。目标设备为 NVIDIA GeForce RTX 5070 Ti Laptop GPU，使用 CUDA Toolkit 12.8 和计算能力 12.0。

## 阶段与完成情况

| 阶段 | 工作项 | 状态 |
|---|---|---|
| 前置收口 | 修复 M1/M7 `BenchmarkRequest` 测试初始化，恢复 Debug/Release 与 Python 基线 | 已完成 |
| 分支隔离 | 从稳定提交创建 `feature/m6-cuda-0726`，不提交或混入其他分支 | 已完成 |
| 条件构建 | 实现 `PARALLELPIX_CUDA=AUTO|ON|OFF`、C++17、cudart 和可覆盖架构 | 已完成 |
| 公共契约 | 提取共享批次预检，增加运行时可用性和实际 CUDA 批大小 | 已完成 |
| CUDA 处理 | 连续打包、单 stream、复用缓冲、融合 Kernel 和 Event 计时 | 已完成 |
| 故障策略 | 显存失败减半并从头重试一次，失败结果不保留指标 | 已完成 |
| M7 接入 | 注册 CUDA 执行器，保留 CPU 结果和既有 CLI/CSV 契约 | 已完成 |
| 测试联调 | CPU-only、真机 Debug/Release、Sanitizer、CLI、CSV 和仪表板 | 已完成 |
| 文档闭环 | 设计、实施、测试记录、总计划、架构和 README | 已完成 |

## 构建命令

CPU-only：

```powershell
cmake -S . -B build/m6-cpu -G "Visual Studio 17 2022" -A x64 `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DPARALLELPIX_CUDA=OFF
```

CUDA：

```powershell
cmake -S . -B build/m6-cuda -G "Visual Studio 17 2022" -A x64 `
  -T "cuda=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.8" `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DPARALLELPIX_CUDA=ON `
  -DCMAKE_CUDA_ARCHITECTURES=120
cmake --build build/m6-cuda --config Release
ctest --test-dir build/m6-cuda -C Release --output-on-failure
```

如果当前终端启动于 Toolkit 安装之前，需要重新打开终端，或在配置命令前设置 `CUDA_PATH`/使用上述 `-T cuda=...`。

## 验收矩阵

### 构建与可用性

- `AUTO` 无 Toolkit：生成 CPU-only 工程；
- `ON` 无 Toolkit：配置失败并给出明确错误；
- `OFF`：不构建、不链接 CUDA；
- CUDA-enabled 且无设备：运行时跳过 CUDA，CPU 后端继续；
- Windows 可执行目录包含 CUDA Runtime DLL。

### 正确性

- 非法配置、空批次、异常图片和水印不匹配；
- 宽图、长图、奇数裁剪和不同分辨率；
- 黑、白、渐变图片；
- 批大小 1、4、8；
- Sequential 对照最大通道误差不超过 1。

### 故障

- 可注入设备分配上限稳定触发 `cudaErrorMemoryAllocation`；
- 首次失败、批大小减半后成功；
- 减半后再次失败；
- 回退丢弃失败尝试的输出、指标和进度；
- CUDA 错误不伪造 H2D、Kernel、D2H。

### 集成与性能

- Debug/Release CTest；
- Python 仪表板回归；
- Compute Sanitizer 小批次 memcheck；
- `sequential,cuda` 全成功退出 0；
- `sequential,openmp,cuda` 中 OpenMP 跳过并退出 2；
- Release 图片数量 1/4/8、CUDA 批大小 1/4/8；
- 每组预热 2 次、正式 5 次；
- 27 列 CSV、实际批大小和 CUDA 三段时间完整；
- 仪表板 Overview、CUDA throughput focus 与 CUDA timing 图可生成。

## 关闭条件

- 全部自动化测试无失败；
- 真机逐像素验证通过；
- Compute Sanitizer 无内存错误；
- CLI 退出码和降级行为符合 M2/M7 契约；
- `git diff --check` 通过；
- 不修改生产配置、不引入数据库、不改变 CLI 参数或 CSV 列数；
- 未经用户确认不执行 `git add`、`git commit` 或 `git push`。
