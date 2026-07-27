# M5 OpenMP 后端测试记录

## 当前状态

已执行。2026-07-27 完成 Debug/Release 双配置构建、CTest、Python 回归，并在 999 张真实商品图数据集上完成性能实验。M5 单元测试 6/6 通过，性能实验 6/6 配置成功且逐像素验证全部通过，16 线程加速比 8.337。

本轮实验暴露了一个测量方法问题（见「测量有效性」一节），最终采用的是在受控状态下重测的数据；被判定无效的一批数据未纳入本文。

## 执行环境

| 项 | 值 |
|------|------|
| 日期 | 2026-07-27 |
| 操作系统 | Windows 11 Home China 10.0.22631 |
| CPU | AMD Ryzen 7 5800H，8 物理核 / 16 逻辑核 |
| 内存 | 15.9 GB |
| 工具链 | Visual Studio 生成工具 2022 17.14.37516.0，MSVC 14.44.35207，x64 |
| 构建目录 | `build/merged`（合并上游 M6 后的代码） |
| CMake 选项 | `-G "Visual Studio 17 2022" -A x64 -DPARALLELPIX_CUDA=OFF` |
| 依赖 | vcpkg manifest，OpenCV 4.12.0（core/jpeg/png） |
| OpenMP | MSVC `-openmp`，版本 2.0 |
| 数据集 | `data/benchmark/abo-products-highres-1000`，**999 张** JPEG，343 MB，平均 352 KB，分辨率混合 |
| 构建结果 | Debug 与 Release 均全量通过 |

> 注一：数据集目录名含 `1000`，实际文件数为 999。`--image-counts 1000` 会在预检阶段以「可解码图片不足」失败。
>
> 注二：本机曾并存残缺的 MSVC 14.39.33519 并排工具集（缺 `lib\x64`、`lib\x86`），导致链接阶段 `LNK1104: 无法打开文件 MSVCRTD.lib`。卸载该组件后 v143 默认工具集回到 14.44.35207，配置命令无需再传 `-T` 或设置 `VCToolsVersion`。

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
| CSV 与 PNG | OpenMP 真实运行 | CSV 含线程数、验证、加速比；PNG 与基准一致 | CSV 27 列齐全，6 行记录的 `validation_passed` 全为 `true`、`max_pixel_error` 全为 0 | 通过 |
| 性能扩展 | 1、2、4、8、最大线程数 | 记录耗时、吞吐量、加速比和效率 | 1/2/4/8/16 五档全部记录了 `compute_ms`、`images_per_second`、`speedup`、`parallel_efficiency`（16 为本机逻辑核心数） | 通过 |

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

合并上游 M6 后的整体回归：CTest Debug 10/10、Release 10/10，Python `pytest -q` 46/46。

## 性能实验

输入取自 999 张真实商品图数据集的前 100 张，预热 2 次、正式测量 5 次，6 个配置全部成功。

| backend | threads | compute_ms | ms/张 | end_to_end_ms | images/s | speedup | efficiency | validation | max_pixel_error |
|---------|---------|-----------|-------|---------------|----------|---------|------------|------------|-----------------|
| sequential | — | 9960.8 | 99.61 | 14216.1 | 10.04 | 1.000 | — | true | 0 |
| openmp | 1 | 10009.6 | 100.10 | 14284.9 | 9.99 | 0.995 | 0.995 | true | 0 |
| openmp | 2 | 5071.8 | 50.72 | 9349.1 | 19.72 | 1.964 | 0.982 | true | 0 |
| openmp | 4 | 2666.2 | 26.66 | 6925.0 | 37.51 | 3.736 | 0.934 | true | 0 |
| openmp | 8 | 1659.9 | 16.60 | 5933.4 | 60.25 | 6.001 | 0.750 | true | 0 |
| openmp | 16 | 1194.7 | 11.95 | 5458.2 | 83.70 | 8.337 | 0.521 | true | 0 |

### 读数说明

- **单线程等价性**：`openmp` 1 线程的加速比为 0.995，与 Sequential 基本相等，说明并行区建立与调度的固定开销可忽略。这一项同时是基线可比性的健全性检验。
- **物理核与超线程的分界**：8 线程（8 物理核）达到 6.001 倍、效率 0.750；继续加到 16 线程（超线程）只涨到 8.337 倍，效率降至 0.521。增益从 8→16 线程明显衰减，符合 SMT 共享执行单元的预期，而非调度缺陷。
- **端到端受 I/O 限制**：`compute_ms` 提升 8.337 倍，但 `end_to_end_ms` 只从 14216 ms 降到 5458 ms（2.6 倍）。JPEG 解码与 PNG 编码不随线程数下降，成为整体瓶颈。吞吐量从 10.04 张/秒提升到 83.70 张/秒。
- **正确性**：全部配置 `validation_passed=true`、`max_pixel_error=0`，即任意线程数下 OpenMP 输出与 Sequential 逐像素完全一致。

### 加速比的独立复现

16 线程 @100 张这一点在三次独立运行中分别测得 **8.22、8.16、8.11**，与本表的 8.337 相互印证。绝对耗时在不同机器状态下差异很大，但加速比稳定。

## 规模上限观察（999 张）

单独测量了 999 张一次性批处理，用于确定当前设计的容量边界：

| backend | threads | compute_ms | ms/张 | end_to_end_ms | img/s | speedup | efficiency | validation | max_pixel_error |
|---------|---------|-----------|-------|---------------|-------|---------|------------|------------|-----------------|
| sequential | — | 172,643.6 | 172.8 | 5,829,956（97.2 分钟） | 5.79 | 1 | — | true | 0 |
| openmp | 16 | 84,853.2 | 84.9 | 275,427（4.6 分钟） | 11.77 | 2.035 | 0.127 | true | 0 |

该组仅测了 16 线程一档，没有 1/2/4/8 曲线。**逐像素正确性在 999 张规模下成立**（`validation_passed=true`、`max_pixel_error=0`），这一结论不受下述计时问题影响。

对比 100 张规模（openmp@16 为 11.95 ms/张），999 张时单张 compute 劣化到 84.9 ms，约 7 倍；加速比**至多** 2.03，远低于 100 张时的 8.337。

原因是批处理将整批图片解码后同时驻留内存。实测该数据集解码为 BGR 三通道后的总量为 **12.41 GB**（平均 12.72 MB/张，最大 18.75 MB/张），叠加 999 张 1024×1024 输出缓冲 **2.93 GB**，合计约 **15.3 GB**，而本机物理内存总量为 15.9 GB。即便不计验证阶段的比对缓冲，整批驻留也已逼近物理内存上限，必然触发换页。

两轮的内存行为差异也印证了这一点：`compute_ms` 只相差 2.03 倍，`end_to_end_ms` 却相差 21 倍（97.2 分钟对 4.6 分钟）。sequential 那一轮有约 95 分钟花在 compute 之外，符合严重换页颠簸的特征；openmp 16 线程那一轮未出现同等程度的颠簸。

作为对照，100 张规模下实测进程工作集约 1.4 GB，未见换页现象。250 张与 500 张的工作集未做测量。

> 该组数据的测量条件为 `--cold-start`（0 预热、1 次测量），因此 `compute_stddev_ms` 为 0 只反映样本量为 1，不代表稳定性。sequential 与 openmp 之间相隔约 97 分钟，受下节所述漂移影响；漂移方向会**高估**后测的 openmp，因此 2.03 应视为上界。结论「999 张规模下加速比大幅塌陷」方向明确，但具体数值不精确。

## 测量有效性

首次尝试的完整矩阵（100/250/500 张 × 1/2/4/8/16 线程，18 个配置，耗时 100.4 分钟）**已判定无效，未纳入本文**。判定依据：

1. 18 行中有 8 行 `parallel_efficiency > 1`，最高 2.276，物理上不可能。
2. 决定性证据是 `openmp` 1 线程：100 张时 230.8 ms/张、250 张时 231.6 ms/张、500 张时 **101.2 ms/张**。单线程不存在任何并行性，批量大小不可能使其快 2.3 倍。
3. 事后在同一二进制、同一输入、同一配置下重测 `sequential` 100 张，得到 99.7 ms/张，而矩阵运行时为 248.3 ms/张——整机在那 100 分钟内处于约 2.5 倍的降速状态，且降速随运行推进逐渐缓解。

根因是方法学缺陷：CLI 的执行顺序为**先跑完全部 sequential 配置，再跑全部 openmp 配置**，三个 sequential 基线集中在最初约 20 分钟，openmp 分布在随后 80 分钟。加速比由跨越约 90 分钟的两个时间点相除得到，当机器状态在此期间漂移时该比值失效。

规避方式：将每个图片规模作为独立的一次 CLI 调用运行，使基线与测量在时间上相邻，并控制单次调用的总时长。本文采用的 100 张曲线即为单次 6.7 分钟的独立调用，其 `openmp` 1 线程 = 0.995 的健全性检验通过。

## 实际执行命令

```powershell
cmake -S . -B build/merged -G "Visual Studio 17 2022" -A x64 `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" `
  -DPARALLELPIX_CUDA=OFF
cmake --build build/merged --config Debug
ctest --test-dir build/merged -C Debug --output-on-failure
cmake --build build/merged --config Release
ctest --test-dir build/merged -C Release --output-on-failure
.\.venv\Scripts\python.exe -m pytest -q

.\build\merged\Release\parallelpix.exe benchmark `
  --input data\benchmark\abo-products-highres-1000 `
  --output output --watermark data\watermark.png `
  --backends sequential,openmp --image-counts 100 --threads 1,2,4,8,16 `
  --warmups 2 --repetitions 5 --csv results\benchmark.csv
```

## 未覆盖与遗留问题

- **更大规模的有效线程曲线仍缺失**：本文的线程扩展曲线仅在 100 张规模上取得。250/500 张的数据因测量漂移作废，需按「测量有效性」一节的方式，以独立调用重跑（500 张一档预计 60~75 分钟）。
- **999 张的精确数值待受控重测**：现有数据只能支撑「加速比大幅塌陷」这一定性结论，2.03 为上界而非测定值。
- **端到端瓶颈未优化**：解码与编码不随线程数下降，16 线程时端到端仅提升 2.6 倍。若要提升整体吞吐，需要让 I/O 与计算重叠，属于后续工作。
- **批处理内存上限未设防**：当前实现将整批解码结果同时驻留内存，999 张即超出本机容量且没有任何预警或分块降级，直接表现为剧烈换页。建议增加批次内存预估与分块处理。
- **CUDA 分支未真机验证**：本次以 `PARALLELPIX_CUDA=OFF` 构建，M7 Pipeline 场景中的 CUDA 路径走的是「后端不可用则跳过」分支。
