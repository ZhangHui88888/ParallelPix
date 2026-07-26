# M5 OpenMP 后端测试记录

## 当前状态

已执行。2026-07-27 完成 Debug/Release 双配置构建、CTest、Python 回归和一次真实 CLI 性能实验。M5 单元测试 6/6 通过（Debug 与 Release 各一次），真实运行 15/15 配置成功且逐像素验证全部通过。遗留项见文末「未覆盖与遗留问题」。

## 执行环境

| 项 | 值 |
|------|------|
| 日期 | 2026-07-27 |
| 操作系统 | Windows 11 Home China 10.0.22631 |
| 工具链 | Visual Studio 生成工具 2022 17.14.37516.0，MSVC 14.44.35207，x64 |
| 构建目录 | `build/m5-v144` |
| CMake 选项 | `-G "Visual Studio 17 2022" -A x64 -DPARALLELPIX_ENABLE_CUDA=OFF` |
| 依赖 | vcpkg manifest，OpenCV 4.12.0（core/jpeg/png） |
| OpenMP | MSVC `-openmp`，版本 2.0 |
| 构建结果 | Debug 与 Release 均全量通过 |

> 注：本机曾并存残缺的 MSVC 14.39.33519 并排工具集（缺 `lib\x64`、`lib\x86`），导致链接阶段 `LNK1104: 无法打开文件 MSVCRTD.lib`。卸载该组件后 v143 默认工具集回到 14.44.35207，配置命令无需再传 `-T` 或设置 `VCToolsVersion`。

## 执行结果

| 场景 | 入参 | 预期 | 实际 | 状态 |
|------|------|------|------|------|
| 调度选择 | 图片数大于/小于线程数 | 分别选择图片级/行级并行 | `choose_scheduling_strategy` 在 (8,4)、(4,4)、(1,1) 返回 `Images`；在 (1,8)、(2,4) 返回 `Rows` | 通过 |
| 多线程一致性 | 1、2、4、8 线程 | 与 Sequential 逐像素完全一致 | 8 张 4×3 合成图，四档线程的输出与 Sequential 在尺寸、通道、stride、`source_path` 和全部像素上逐项相等 | 通过 |
| 小批次行并行 | 1 张图片、4 个线程 | 与 Sequential 逐像素完全一致 | 触发行级并行，输出与 Sequential 逐项相等 | 通过 |
| 顺序与输入不变 | 多张不同来源图片 | 输出顺序、路径稳定，输入未修改 | 8 张批次的 `source_path` 按索引逐项相等；12 张批次运行后输入像素与水印 alpha 均未改变 | 通过 |
| 进度轨迹 | 12 张图片、4 个线程 | 已完成数量单调递增并到达 12 | 回调序列严格递增，末值为 12，每次上报的 `ms_per_image` 均非负 | 通过 |
| 非法线程数 | 0 线程 | 返回 `InvalidConfig`，无部分输出 | `result.ok()` 为 false，问题列表含 `ProcessingIssueCode::InvalidConfig`，未产出图片 | 通过 |
| M7 Pipeline | Sequential+OpenMP+CUDA | CPU 两项成功；CUDA 可用时运行，否则跳过并部分成功 | `parallelpix_m7_tests` 中该用例通过；本次构建关闭 CUDA，CUDA 后端按「本构建不可用」跳过 | 通过（CUDA 分支未在真实设备上验证） |
| CSV 与 PNG | OpenMP 真实运行 | CSV 含线程数、验证、加速比；PNG 与基准一致 | CSV 27 列齐全，15 行记录的 `validation_passed` 全为 `true`、`max_pixel_error` 全为 0 | 通过 |
| 性能扩展 | 1、2、4、8、最大线程数 | 记录耗时、吞吐量、加速比和效率 | 1/2/4/8 四档均记录了 `compute_ms`、`images_per_second`、`speedup`、`parallel_efficiency`；「最大线程数」档未执行 | 部分完成 |

## 单元测试输出

`parallelpix_m5_tests`（Debug 与 Release 各执行一次，结果相同）：

```text
[PASS] OpenMP image scheduling matches Sequential for configured threads
[PASS] OpenMP row scheduling matches Sequential for a small batch
[PASS] OpenMP processor preserves inputs and reports monotonic progress
[PASS] OpenMP processor rejects an invalid thread count
[PASS] OpenMP scheduling prefers image-level parallelism
[PASS] OpenMP scheduling falls back to row parallelism for small batches
6 tests, 0 failures
```

CTest 整体（Debug）10 个测试中 9 个通过，唯一失败为 `parallelpix_m7_tests` 的 `benchmark runner records CUDA phases and effective batch size`，属 M7 范围，见文末遗留问题。Python 回归 `pytest -q` 40/40 通过。

## 性能实验

输入为 6 张真实商品图片（5 张 JPEG + 1 张 PNG），预热 2 次、正式测量 5 次，全部 15 个配置成功，无失败无跳过。

6 张图片一组的结果：

| backend | threads | compute_ms | end_to_end_ms | speedup | parallel_efficiency | validation_passed | max_pixel_error |
|---------|---------|-----------|---------------|---------|---------------------|-------------------|-----------------|
| sequential | — | 679.50 | 1465.61 | 1.000 | — | true | 0 |
| openmp | 1 | 736.87 | 1584.31 | 0.922 | 0.922 | true | 0 |
| openmp | 2 | 414.75 | 1269.06 | 1.638 | 0.819 | true | 0 |
| openmp | 4 | 267.84 | 1120.50 | 2.537 | 0.634 | true | 0 |
| openmp | 8 | 127.16 | 1008.11 | 5.343 | 0.668 | true | 0 |

2 张和 4 张图片两组呈现同样的趋势，8 线程加速比分别为 5.475 和 5.330。

读数说明：`speedup` 基于 `compute_ms`，反映纯处理阶段的并行收益；`end_to_end_ms` 含 JPEG/PNG 解码与编码，这部分不随线程数下降，因此端到端降幅明显小于 `compute_ms` 的降幅。单线程 OpenMP 在 4 张和 6 张两组相对 Sequential 略慢（0.933 与 0.922），为并行区建立与调度的固定开销；2 张一组因批次过小落在噪声范围内（1.024）。

## 实际执行命令

```powershell
cmake -S . -B build/m5-v144 -G "Visual Studio 17 2022" -A x64 `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DPARALLELPIX_ENABLE_CUDA=OFF
cmake --build build/m5-v144 --config Debug
ctest --test-dir build/m5-v144 -C Debug --output-on-failure
cmake --build build/m5-v144 --config Release
.\build\m5-v144\Release\parallelpix_m5_tests.exe
.\.venv\Scripts\python.exe -m pytest -q

.\build\m5-v144\Release\parallelpix.exe benchmark `
  --input <图片目录> --output output --watermark data\watermark.png `
  --backends sequential,openmp --image-counts 2,4,6 --threads 1,2,4,8 `
  --warmups 2 --repetitions 5 --csv results\benchmark.csv
```

## 未覆盖与遗留问题

- **数据集规模不足**：本次只有 6 张图片可用，`--image-counts` 被迫取 2/4/6，而非设计中的 10/50/100。加速比趋势可信，但并行效率在 4/8 线程降到 0.63~0.67 很可能由批次过小、调度开销占比过高导致，不能代表 1000 张规模下的表现。需要补齐 `data/benchmark/sop-products-1000/` 后重跑「性能扩展」一行。
- **「最大线程数」档未执行**：仅覆盖 1/2/4/8，未按本机逻辑核心数追加一档。
- **CUDA 分支未真机验证**：本次以 `PARALLELPIX_ENABLE_CUDA=OFF` 构建，M7 Pipeline 场景中的 CUDA 路径走的是「后端不可用则跳过」分支。
- **M7 遗留失败**：`benchmark runner records CUDA phases and effective batch size` 期望 `summary.succeeded == 2` 未满足。该用例使用假执行器，不依赖真实 GPU，因此与本次关闭 CUDA 无关，属 runner 成功/跳过计数与测试之间的不一致，待 M7 单独处理。
- **构建期修复**：`tests/cpp/planning/test_benchmark_plan.cpp` 的聚合初始化缺少 `BenchmarkRequest` 新增的 `cold_start` 字段，导致 `parallelpix_m2_tests` 编译失败（C2440）。已补齐，与 M5 功能无关。
