# ParallelPix 项目演讲稿 / Presentation Script

> 这是照读版：只读“中文讲稿”或“English script”，“页面提示”不用读。  
> Read-aloud version: read only the Chinese or English script. Slide cues are not spoken.  
> 编号提醒（不用读）：演讲中的 M3 是 Control；仓库中的 Control 属于 M2，正式 M3 是图片 I/O。

---

## 第 1 页：封面 / Slide 1: Cover

### 页面提示（不用念）/ Slide cue (do not read)

# ParallelPix

### 中文讲稿（照读）

大家好。  
我今天介绍的项目叫 ParallelPix。

它用同一套图片处理流程，比较 Sequential、OpenMP 和 CUDA。

### English script (read aloud)

Hello everyone.  
Today I will present ParallelPix.

It compares Sequential, OpenMP, and CUDA using the same image-processing pipeline.

---

## 第 2 页：业务是什么，为什么选择它 / Slide 2: Business Problem and Topic Selection

### 页面提示（不用念）/ Slide cue (do not read)

**业务：批量标准化电商商品图片**  
**Business: Batch standardization of e-commerce product images**

```text
原始商品图 / Raw product image
  → 中心裁剪 / Center crop
  → 双线性缩放到 1024×1024 / Bilinear resize to 1024×1024
  → 固定 1.10 亮度增强 / Fixed 1.10 brightness enhancement
  → 右下角水印 / Bottom-right watermark
  → 无损 PNG / Lossless PNG
```

**选择原因：每张图片都执行相同流水线，图片之间相互独立。**  
**Why this topic: Every image follows the same pipeline, and images are independent.**

### 中文讲稿（照读）

这个项目做的是商品图片批处理。

每张图片都会经过四步：  
中心裁剪、双线性缩放、亮度乘以 1.10、再加水印。

我选择这个主题，是因为每张图片做的事情都一样，而且图片之间互不影响。  
所以它很适合并行计算。

### English script (read aloud)

This project processes product images in batches.

Every image follows four steps:  
center crop, bilinear resize, brightness multiplied by 1.10, and watermark blending.

I chose this topic because every image performs the same work, and images are independent.  
That makes the task suitable for parallel computing.

---

## 第 3 页：项目模块总览 / Slide 3: Module Overview

### 页面提示（不用念）/ Slide cue (do not read)

```text
M1 Dashboard
      ↓
M2 CLI
      ↓
M3 Control
      ↓
M4 统一单图流水线 / Unified single-image pipeline
   Sequential baseline
      ├── M5 OpenMP
      └── M6 CUDA
      ↓
验证、Benchmark CSV、Dashboard 图表
Validation, benchmark CSV, and dashboard charts
```

| 模块 / Module | 主要职责 / Main responsibility |
| --- | --- |
| M1 Dashboard | 配置实验、启动 CLI、展示日志和结果 / Configure experiments, start the CLI, and display logs and results |
| M2 CLI | 定义可重复执行的命令与参数边界 / Define a reproducible command and parameter boundary |
| M3 Control | 生成实验矩阵、选择后端、统一错误和退出状态 / Build the experiment matrix, select backends, and normalize errors and exit states |
| M4 Pipeline | 定义唯一的图片处理语义与串行正确性基线 / Define the single processing semantics and sequential correctness baseline |
| M5 OpenMP | 在共享内存 CPU 上并行图片或输出行 / Parallelize images or rows on a shared-memory CPU |
| M6 CUDA | 在 GPU 上把输出像素映射为大量线程 / Map output pixels to a large number of GPU threads |

### 中文讲稿（照读）

整个项目可以分成六块。

M1 是 Dashboard。  
M2 是 CLI。  
M3 是 Control。  
M4 定义图片处理流程和 Sequential 基线。  
M5 使用 OpenMP。  
M6 使用 CUDA。

简单来说，前面三个模块负责控制实验，后面三个模块负责图片计算。

### English script (read aloud)

The project has six parts.

M1 is the Dashboard.  
M2 is the CLI.  
M3 is Control.  
M4 defines the image pipeline and the Sequential baseline.  
M5 uses OpenMP.  
M6 uses CUDA.

In short, the first three parts control the experiment, and the last three perform image computation.

---

## 第 4 页：M1 Dashboard 做什么 / Slide 4: What M1 Dashboard Does

### 页面提示（不用念）/ Slide cue (do not read)

**M1 是实验控制台和结果浏览器，不是被测计算后端。**  
**M1 is an experiment console and result viewer, not a measured compute backend.**

- 配置输入、输出、水印和 CSV 路径 / Configure input, output, watermark, and CSV paths
- 选择 Sequential、OpenMP、CUDA / Select Sequential, OpenMP, and CUDA
- 配置图片数、线程数和 CUDA batch / Configure image counts, thread counts, and CUDA batch size
- 支持 Demo / Local CLI / Support Demo and Local CLI modes
- 展示状态、日志、历史 run、可扩展性和 CUDA 分阶段时间 / Display status, logs, run history, scalability, and CUDA phase timing

### 中文讲稿（照读）

这一页是 M1，也就是 Dashboard。

我可以在这里选择图片、后端、线程数和 CUDA batch。  
点击运行以后，它会启动 C++ 程序，再把结果画成图表。

Dashboard 只负责操作和展示。  
它不负责图片计算。

### English script (read aloud)

This is M1, the Dashboard.

Here I can select the images, backend, thread count, and CUDA batch size.  
After I start the experiment, it runs the C++ program and displays the results as charts.

The Dashboard controls and displays the experiment.  
It does not process the images.

---

## 第 5 页：M2 CLI 是什么，为什么单独成模块 / Slide 5: What M2 CLI Is and Why It Is Separate

### 页面提示（不用念）/ Slide cue (do not read)

```powershell
parallelpix benchmark `
  --input data/images `
  --output output `
  --watermark data/watermark.png `
  --backends sequential,openmp,cuda `
  --image-counts 10,50,100 `
  --threads 1,2,4,8 `
  --cuda-batches 1,4,8 `
  --warmups 2 --repetitions 5 `
  --csv results/benchmark.csv --append
```

**CLI = 稳定、可脚本化、可复现的实验入口。**  
**CLI = A stable, scriptable, and reproducible experiment entry point.**

### 中文讲稿（照读）

M2 是 CLI，也就是命令行入口。

Dashboard 里的选择，最后都会变成一条命令。  
这条命令包含图片路径、后端、线程数和 batch size。

单独做 CLI 的好处是，同一个实验可以重复运行，也可以直接写进脚本。

### English script (read aloud)

M2 is the CLI, or command-line interface.

The settings from the Dashboard are converted into one command.  
The command contains the image path, backend, thread count, and batch size.

A separate CLI makes the same experiment repeatable and scriptable.

---

## 第 6 页：M3 Control 是什么，为什么单独成模块 / Slide 6: What M3 Control Is and Why It Is Separate

### 页面提示（不用念）/ Slide cue (do not read)

```text
合法 CLI 请求 / Valid CLI request
  → BenchmarkRequest
  → 规范化后端 / Normalize backends
  → 展开实验矩阵 / Expand the experiment matrix
  → 调用统一 Pipeline / Call the unified pipeline
  → 校验 WorkflowSummary / Validate WorkflowSummary
  → 日志与退出码 / Logs and exit code
```

**Control 决定“做哪些实验”，后端决定“怎样计算”。**  
**Control decides which experiments to run; the backend decides how to compute them.**

### 中文讲稿（照读）

M3 是 Control。

CLI 负责接收命令，Control 负责决定要跑哪些实验。

比如我选择 OpenMP，Control 会自动加入 Sequential 作为基线。  
然后它按照图片数和线程数，生成完整的实验列表，再调用对应的后端。

### English script (read aloud)

M3 is Control.

The CLI receives the command, while Control decides which experiments to run.

For example, when I select OpenMP, Control automatically adds Sequential as the baseline.  
It then creates the experiment list from the image counts and thread counts and calls the selected backend.

---

## 第 7 页：M4 与单张图片的业务流水线 / Slide 7: M4 and the Single-Image Business Pipeline

### 页面提示（不用念）/ Slide cue (do not read)

```text
文件系统与 OpenCV I/O（正式 M3）
File system and OpenCV I/O (official M3)
  ↓ 连续三通道 BGR Image / Contiguous three-channel BGR image
中心裁剪 / Center crop
  ↓
半像素双线性缩放 / Half-pixel bilinear resize
  ↓
固定亮度增强 × 1.10 / Fixed brightness enhancement × 1.10
  ↓
水印 Alpha 混合 / Watermark alpha blending
  ↓
内存 Image → PNG / In-memory image → PNG
```

```text
source = crop_origin
       + (output_coordinate + 0.5) × scale
       - 0.5
```

### 中文讲稿（照读）

M4 是一张图片的完整处理流程。

第一步是中心裁剪。  
第二步是双线性缩放。  
第三步是把亮度乘以 1.10。  
最后一步是加水印。

Sequential 会按照这个顺序，一个像素一个像素地处理。  
后面的 OpenMP 和 CUDA，做的还是同一件事，只是并行方式不同。

### English script (read aloud)

M4 is the complete pipeline for one image.

First, the image is center-cropped.  
Second, it is resized with bilinear interpolation.  
Third, brightness is multiplied by 1.10.  
Finally, the watermark is added.

Sequential processes the pixels one by one in this order.  
OpenMP and CUDA perform the same work with different forms of parallelism.

---

## 第 8 页：M5 OpenMP 的核心原理 / Slide 8: Core Principles of M5 OpenMP

### 页面提示（不用念）/ Slide cue (do not read)

**OpenMP：共享地址空间中的多核 CPU 并行**  
**OpenMP: Multi-core CPU parallelism in a shared address space**

| 条件 / Condition | 并行粒度 / Granularity | 调度 / Scheduling |
| --- | --- | --- |
| `image_count >= thread_count` | 图片级 / Image level | `dynamic, 1` |
| `image_count < thread_count` | 单图行级 / Row level within one image | `static` |

**不使用嵌套并行。/ No nested parallelism.**

### 中文讲稿（照读）

M5 使用 OpenMP，也就是让多个 CPU 线程一起工作。

如果图片很多，就让每个线程处理不同的图片。  
如果图片很少，就把一张图片按行分给多个线程。

这两种方式只选一种，不会同时使用。

### English script (read aloud)

M5 uses OpenMP, which means multiple CPU threads work together.

When there are many images, different threads process different images.  
When there are only a few images, one image is divided by rows across the threads.

The program selects one of these strategies, not both.

---

## 第 9 页：M5 线程分工、同步与结果 / Slide 9: M5 Work Assignment, Synchronization, and Results

### 页面提示（不用念）/ Slide cue (do not read)

```text
图片级 / Image level:
线程 → 完整图片 → 独立输出
Thread → complete image → separate output

行级 / Row level:
线程 → 互不重叠的输出行
Thread → disjoint output rows

缩放与亮度 → Barrier → 水印
Resize and brightness → Barrier → Watermark
```

**无像素锁：每个线程写入互不重叠的输出区域。**  
**No pixel locks: each thread writes to a disjoint output region.**

### 中文讲稿（照读）

OpenMP 的关键是，每个线程只写自己的图片，或者自己的几行。  
所以线程之间不会同时修改同一个像素，也不需要给每个像素加锁。

在六张图片的测试里，Sequential 用了 679.50 毫秒。  
OpenMP 八线程用了 127.16 毫秒。  
加速比是 5.343 倍。

### English script (read aloud)

The key point is that each OpenMP thread writes to its own image or its own rows.  
Threads do not modify the same pixel, so pixel-level locks are not needed.

For six images, Sequential took 679.50 milliseconds.  
OpenMP with eight threads took 127.16 milliseconds.  
The speedup was 5.343 times.

---

## 第 10 页：M6 CUDA 的 Host–Device 数据链路 / Slide 10: M6 CUDA Host–Device Data Path

### 页面提示（不用念）/ Slide cue (do not read)

```text
变长输入图片 / Variable-sized input images
  → Host 连续打包 + 每图 Descriptor
    Contiguous Host packing + one descriptor per image
  → H2D
  → Fused Kernel
  → D2H
  → 固定尺寸输出 Image / Fixed-size output images
```

```text
Descriptor = {
  input_offset, width, height, stride,
  crop_x, crop_y, crop_width, crop_height
}
```

```text
Batch Size：一次提交的图片数，不是越大越好
Batch Size: images submitted at once; bigger is not always better

目标：在可用显存内，取得最高吞吐量且满足延迟要求
Goal: highest throughput within available VRAM and acceptable latency
```

### 中文讲稿（照读）

M6 使用 CUDA，也就是把计算交给 GPU。

CPU 会先把图片打包，再传到 GPU。  
GPU 处理完成以后，再把结果传回 CPU。

这里的 batch size，表示一次送多少张图片。  
它不是越大越好。  
太大会占用更多显存，所以需要通过测试来选择。

### English script (read aloud)

M6 uses CUDA, which moves the computation to the GPU.

The CPU first packs the images and transfers them to the GPU.  
After processing, the results are transferred back to the CPU.

Batch size means how many images are submitted at once.  
A larger batch is not always better because it uses more GPU memory, so the value must be measured.

---

## 第 11 页：M6 如何把一个输出像素映射为一个 GPU 线程 / Slide 11: Mapping One Output Pixel to One GPU Thread

### 页面提示（不用念）/ Slide cue (do not read)

```text
block = (16, 16, 1)
grid.x → 输出 x / output x
grid.y → 输出 y / output y
grid.z → batch 中的图片 / image in the batch

一个线程 → 一个输出像素 → 三个 BGR 通道
One thread → one output pixel → three BGR channels
```

### 中文讲稿（照读）

在 CUDA 里，一个 GPU 线程负责一个输出像素。

Grid 的 x 和 y 表示像素坐标。  
z 表示 batch 里的第几张图片。

每个线程会一次完成缩放、亮度和水印。  
不同线程写不同的像素，所以它们可以同时运行。

### English script (read aloud)

In CUDA, one GPU thread processes one output pixel.

The x and y dimensions of the grid represent pixel coordinates.  
The z dimension selects an image in the batch.

Each thread performs resize, brightness, and watermark blending.  
Different threads write different pixels, so they can run in parallel.

---

## 第 12 页：M6 的计时、显存回退与真实结果 / Slide 12: M6 Timing, Memory Fallback, and Measured Results

### 页面提示（不用念）/ Slide cue (do not read)

**8 张图片，CUDA batch 8，同一 run，2 次预热 + 5 次测量中位数**  
**8 images, CUDA batch 8, same run, median of 2 warm-ups and 5 measured repetitions**

| 指标 / Metric | Sequential | CUDA |
| --- | ---: | ---: |
| Compute | 390.8626 ms | 26.7116 ms |
| End-to-end | 634.1155 ms | 222.7229 ms |
| Compute speedup | 1.00× | 14.6327× |
| 最大像素误差 / Maximum pixel error | 0 | 0 |

```text
CUDA compute 26.7116 ms
├── H2D      5.1865 ms
├── Kernel   7.4422 ms
├── D2H      3.1532 ms
└── Host 与其他开销 / Host and other overhead ≈ 10.9298 ms
```

### 中文讲稿（照读）

这一页是 CUDA 的测试结果。

八张图片，Sequential 用了 390.8626 毫秒。  
CUDA 用了 26.7116 毫秒。  
计算部分加速了 14.6327 倍。

但是端到端只加速了大约 2.85 倍。  
因为读取图片、保存图片和数据传输，也需要时间。

### English script (read aloud)

This slide shows the CUDA result.

For eight images, Sequential took 390.8626 milliseconds.  
CUDA took 26.7116 milliseconds.  
The compute speedup was 14.6327 times.

However, end-to-end speedup was only about 2.85 times because image loading, image saving, and data transfer also take time.

---

## 第 13 页：结论 / Slide 13: Conclusion

### 页面提示（不用念）/ Slide cue (do not read)

**同一业务流水线，三种计算路径。**  
**One business pipeline, three compute paths.**

- Sequential：定义正确性与性能基线 / Defines the correctness and performance baseline
- OpenMP：并行粒度与调度决定多核效率 / Granularity and scheduling determine multi-core efficiency
- CUDA：大量像素线程带来计算加速，数据移动与 Host 开销决定端到端收益 / Many pixel threads accelerate computation, while data movement and Host overhead determine end-to-end benefit

**正确性、加速、开销和适用边界必须一起解释。**  
**Correctness, speedup, overhead, and applicability boundaries must be explained together.**

### 中文讲稿（照读）

最后总结一下。

Sequential 是基线。  
OpenMP 使用多个 CPU 线程。  
CUDA 使用大量 GPU 线程。

在这次测试中，CUDA 的计算速度最快。  
但是实际总时间，还会受到数据传输和文件读写的影响。

谢谢大家。

### English script (read aloud)

To summarize:

Sequential is the baseline.  
OpenMP uses multiple CPU threads.  
CUDA uses many GPU threads.

In this experiment, CUDA had the fastest compute time.  
However, total application time is still affected by data transfer and file I/O.

Thank you.

---

## 备用提醒（不用念）/ Backup Notes (Do Not Read)

- 演讲中的 M3 是 Control；仓库正式 M3 是图片 I/O。
- 亮度乘以 1.10 是固定实验参数，不是自动曝光；原本很亮的图片可能饱和。
- OpenMP 只并行带指令的循环，不会自动并行整个程序。
- CUDA 的 batch size 是一次提交的图片数；Block 固定为 16×16。
- Kernel 时间不等于完整 CUDA 时间，数据传输和文件读写也有开销。

## 备用回答：老师问 Hybrid 时再说 / Backup Answer: Hybrid

### 中文（照读）

如果同时使用 CPU 和 GPU，我会按整张图片划分任务，而不是把每张图切成两部分。

因为整图划分只需要在一批任务结束时同步，额外开销更小。

根据现有数据，可以先测试 GPU 处理 85% 到 90%，CPU 处理 10% 到 15%。

图片很少时，全部交给 CUDA 可能更快；图片很多时，Hybrid 才可能有优势。

### English (read aloud)

If the CPU and GPU are used together, I would divide the work by complete images instead of splitting every image.

This requires less coordination because synchronization happens only after a batch.

Based on the current results, I would first test 85 to 90 percent on the GPU and 10 to 15 percent on the CPU.

For a small number of images, CUDA alone may be faster. Hybrid processing is more likely to help with a large batch.
