# M6 CUDA 后端测试记录

## 认证要求

无。测试仅使用本机 CLI、GPU、测试图片和本地性能仪表板，不访问远程服务。

## 前置条件与环境

| 项目 | 实际值 |
|---|---|
| 日期 | 2026-07-26 |
| 操作系统/构建器 | Windows、Visual Studio 2022 17.14、MSVC 19.44 |
| CMake | 3.31.6-msvc6 |
| CUDA Toolkit | 12.8.61 |
| 显卡 | NVIDIA GeForce RTX 5070 Ti Laptop GPU |
| 驱动 | 572.84 |
| 计算能力 | 12.0（`sm_120`） |
| 显存 | 12227 MiB |
| 输入 | `data/benchmark/abo-products-highres-1000`，1000 张商品图 |
| 水印 | `data/watermark.png` |
| 输出 | 1024×1024 BGR PNG |
| 性能配置 | Release，预热 2 次、正式 5 次 |

## 自动化与故障测试

| 场景 | 入参/方法 | 预期 | 实际 | 状态 |
|---|---|---|---|---|
| CPU-only OFF | `PARALLELPIX_CUDA=OFF` | 不构建 CUDA，既有流程全绿 | Debug CTest 9/9 | 通过 |
| AUTO 无 Toolkit | 安装前配置 `AUTO` | 生成 CPU-only 工程 | 配置成功 | 通过 |
| ON 无 Toolkit | 安装前配置 `ON` | 配置失败 | 明确提示未发现 CUDA compiler | 通过 |
| CUDA Debug | `sm_120` Debug | 全部模块与进程测试通过 | CTest 10/10 | 通过 |
| CUDA Release | `sm_120` Release | 全部模块与进程测试通过 | CTest 10/10 | 通过 |
| 共享预检 | 非法配置、异常图片、Alpha 不匹配 | 返回对应 issue | 与预期一致 | 通过 |
| 无设备 | 内部 Runtime 注入设备发现失败 | Processor 不可用，M7 可跳过 | 与预期一致 | 通过 |
| API/Kernel 错误 | 内部 Runtime 分别注入 API 与 Kernel 启动失败 | 返回失败且无阶段指标 | 与预期一致 | 通过 |
| 像素一致性 | 黑/白/渐变、不同宽高、奇数裁剪 | 最大误差 ≤1 | 最大误差 0 | 通过 |
| 批大小 | 1、4、8 | 输出顺序、进度和阶段计时正确 | 与预期一致 | 通过 |
| 首次显存失败 | 内部 Runtime 分配上限 | 批大小 4→2，从头重试成功 | 实际批大小 2 | 通过 |
| 二次显存失败 | 更低分配上限 | 返回失败且无阶段指标 | 与预期一致 | 通过 |
| 回退进度 | 第二批分配失败 | 丢弃首次尝试进度 | 仅报告 2/4/6/8 | 通过 |
| 轨迹成功缓冲 | 批大小 2 | 完整尝试成功后交付 2/4 | 与预期一致 | 通过 |
| 轨迹计时隔离 | Runner 捕获批次样本 | `compute_ms` 与 `end_to_end_ms` 停止后才输出轨迹日志 | 与预期一致 | 通过 |
| Compute Sanitizer | Release M6 tests、memcheck | 无越界和非法访问 | 9/9，`ERROR SUMMARY: 0 errors` | 通过 |
| Python 仪表板 | `pytest -q` | CSV/图表、无实时模式、完成后轨迹和单次冷启动回归全绿 | 46/46 | 通过 |
| C++ Debug/Release | `ctest --output-on-failure` | 所有模块与 CLI 烟测全绿 | Debug 10/10；Release 10/10 | 通过 |

Compute Sanitizer 在 Windows WDDM 下需要 Toolkit 自带的管理员调试接口。测试前临时将 `HKLM\SOFTWARE\NVIDIA Corporation\GPUDebugger\EnableInterface` 设为 1，完成后已恢复为 0。

## CLI 与降级验收

| 请求 | 预期 | 实际 | 状态 |
|---|---|---|---|
| `--backends sequential,cuda` | 两个后端成功，退出 0 | `succeeded=2 skipped=0 code=0` | 通过 |
| `--backends sequential,openmp,cuda` | Sequential/CUDA 成功，OpenMP 跳过，退出 2 | `succeeded=2 skipped=1 code=2` | 通过 |
| CUDA Runtime 不可用替身 | CUDA 不执行，CPU 结果保留 | `BackendUnavailable`，CUDA calls=0 | 通过 |
| CUDA CSV | 27 列、实际批大小、三段时间非空 | 12 行均满足 | 通过 |
| M1 图表 | 读取真实 CSV 并生成 CUDA 图表 | Overview 3 张、throughput focus 和 timing 均非空 | 通过 |
| 单次冷启动 | 一个新 CLI 进程运行 Sequential/CUDA 矩阵 | 每配置 0 次预热、1 次正式运行，无额外 Sequential 探测 | 请求契约测试通过 | 通过 |

## Release 性能结果

原始记录保存在本机 `results/m6-cuda-20260726.csv`。以下均为 5 次正式运行的中位数；本轮没有发生显存回退，实际批大小等于请求值。

| 后端 | 图片数 | 实际批大小 | compute_ms | speedup | H2D ms | Kernel ms | D2H ms | 最大误差 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Sequential | 1 | — | 44.8119 | 1.0000 | — | — | — | 0 |
| CUDA | 1 | 1 | 3.3710 | 13.2934 | 0.7184 | 0.8846 | 0.4215 | 0 |
| CUDA | 1 | 4 | 3.3466 | 13.3903 | 0.7695 | 0.8874 | 0.4269 | 0 |
| CUDA | 1 | 8 | 4.0619 | 11.0323 | 0.9525 | 0.8878 | 0.4372 | 0 |
| Sequential | 4 | — | 160.1717 | 1.0000 | — | — | — | 0 |
| CUDA | 4 | 1 | 17.4923 | 9.1567 | 7.1397 | 4.1574 | 1.6817 | 0 |
| CUDA | 4 | 4 | 23.3394 | 6.8627 | 9.3714 | 3.4409 | 3.9811 | 0 |
| CUDA | 4 | 8 | 20.5421 | 7.7972 | 7.6879 | 3.4340 | 3.8439 | 0 |
| Sequential | 8 | — | 390.8626 | 1.0000 | — | — | — | 0 |
| CUDA | 8 | 1 | 30.6622 | 12.7474 | 8.6316 | 7.1034 | 3.3509 | 0 |
| CUDA | 8 | 4 | 31.2697 | 12.4997 | 7.2407 | 8.5565 | 3.3772 | 0 |
| CUDA | 8 | 8 | 26.7116 | 14.6327 | 5.1865 | 7.4422 | 3.1532 | 0 |

这些数字只描述本机当次运行，不作为自动化性能门槛。三段 CUDA Event 时间之和可小于完整 `compute_ms`，因为后者还包含主机打包、输出对象组装和调用开销。

## 结论

M6 已在目标 Blackwell Laptop GPU 上完成编译、像素正确性、显存回退、分阶段计时、CLI/M7 和仪表板闭环。M5 OpenMP 仍未实现；整个课程项目最终验收前仍需完成第二类并行组件。
