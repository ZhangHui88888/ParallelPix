# ParallelPix 代码顺序讲解稿

> 用途：现场答辩的代码讲解环节。  
> 建议时长：完整版约 12～15 分钟；只读每节的“讲稿”并跳过“可选补充”，约 8～10 分钟。  
> 使用方式：按本文顺序打开代码；点击每节标题或“代码跳转”即可定位到对应源码行。  
> 讲解主线：程序入口 → 实验计划 → Benchmark Runner → 公共像素语义 → Sequential → OpenMP → CUDA → 正确性与性能记录。

## 一句话总览

> ParallelPix 不是三套互不相关的图片程序，而是一条统一的图片处理与 Benchmark 流水线。Sequential、OpenMP 和 CUDA 只替换计算后端，因此三者可以使用相同输入、相同处理语义和相同验证规则进行比较。

运行链如下：

```text
CLI main
  → Controller
  → BenchmarkPlan
  → Benchmark Runner
  → Backend Executor
      ├── Sequential
      ├── OpenMP
      └── CUDA
  → PNG 验证
  → 统计、Speedup、CSV
```

---

## 1. 从程序入口开始：CLI 只负责把控制权交给 Pipeline

### 代码跳转

- [`src/cli/main.cpp` 第 17～28 行：创建 Pipeline 并进入 Controller](../../src/cli/main.cpp#L17-L28)
- [`src/cli/main.cpp` 第 75～96 行：Windows UTF-8 参数入口](../../src/cli/main.cpp#L75-L96)
- [`src/controller/controller.cpp` 第 115～156 行：解析参数、建立计划并执行](../../src/controller/controller.cpp#L115-L156)

### 屏幕操作

先打开 `main.cpp` 第 17 行，指向第 26～28 行；随后点击 `controller.cpp` 第 121、139 和 156 行。

### 讲稿

“我先从真实程序入口开始。

`main.cpp` 本身没有图片算法。它只做两件事：把命令行参数统一成 UTF-8，然后创建 Benchmark Pipeline，调用 `run_cli`。

进入 Controller 以后，第 121 行先解析参数。参数合法后，第 139 行把用户请求转换成实验计划；第 156 行再把计划交给统一 Pipeline 执行。

这里刻意把参数解析、实验编排和图片计算分开。这样 Dashboard、命令行和自动化脚本最终都能进入同一条可复现的实验路径。”

### 过渡

“接下来要看的是，一条 CLI 请求怎样变成多个 Sequential、OpenMP 和 CUDA 实验。”

---

## 2. 实验计划：先建立同规模 Sequential 基线，再展开并行配置

### 代码跳转

- [`src/planning/benchmark_plan.cpp` 第 29～49 行：规范化后端顺序](../../src/planning/benchmark_plan.cpp#L29-L49)
- [`src/planning/benchmark_plan.cpp` 第 54～101 行：展开实验矩阵](../../src/planning/benchmark_plan.cpp#L54-L101)
- [`src/pipeline/benchmark_pipeline.cpp` 第 27～37 行：把执行器交给 Runner](../../src/pipeline/benchmark_pipeline.cpp#L27-L37)

### 屏幕操作

先指向 `normalize_backends()`，再下移到第 72～98 行的三类实验生成逻辑。

### 讲稿

“这里是实验矩阵的生成。

如果用户只选择 OpenMP 或 CUDA，第 31～39 行仍然会自动加入 Sequential。原因是 speedup 必须有同一图片规模下的串行基线。

第 72 行开始按固定顺序展开实验：先生成每个图片规模的 Sequential，再生成 OpenMP 的不同线程数，最后生成 CUDA 的不同 batch size。

所以后面不会拿不同规模、不同运行轮次的数据随意相除。实验顺序本身就在保护比较的公平性。”

### 过渡

“计划建立以后，Runner 会统一准备输入，再为每个配置选择对应后端。”

---

## 3. Benchmark Runner：公共 I/O 只做一次设计，计算后端可以替换

### 代码跳转

- [`src/benchmark/runner/runner.cpp` 第 77～128 行：输出、目录、图片与水印预检](../../src/benchmark/runner/runner.cpp#L77-L128)
- [`src/benchmark/runner/runner.cpp` 第 161～186 行：注册执行器并开始实验循环](../../src/benchmark/runner/runner.cpp#L161-L186)
- [`src/benchmark/runner/runner.cpp` 第 216～254 行：取得同规模基线并测量实验](../../src/benchmark/runner/runner.cpp#L216-L254)
- [`src/benchmark/runner/runner.cpp` 第 276～306 行：保存基线并写入结果](../../src/benchmark/runner/runner.cpp#L276-L306)

### 屏幕操作

重点展示第 112～137 行的输入预检、第 183 行的基线表，以及第 216～254 行。

### 讲稿

“Runner 是三种后端的公共外壳。

它先检查 CSV 目标和输出目录，再扫描图片、读取水印，并按最大实验规模做一次预检。之后，第 161 行把 Sequential、OpenMP 和 CUDA 执行器按后端类型登记。

第 183 行有一张按图片数量保存的 Sequential 时间表。运行并行后端之前，第 216～231 行必须先找到相同 `image_count` 的基线；找不到就跳过，而不是发布一个没有依据的 speedup。

这体现了项目的核心架构：数据、验证和统计路径相同，只有 Backend Executor 被替换。”

### 可选补充

“CUDA 未编译或运行时不可用时，Runner 会把对应配置标记为 skipped，并保留 CPU 结果，不会伪装成 CUDA 成功。”

### 过渡

“在比较三种后端以前，必须先定义三者共同遵守的像素语义。”

---

## 4. 公共语义：双线性缩放是三个后端必须一致的核心热点

### 代码跳转

- [`src/common/resize/bilinear.cpp` 第 26～58 行：半像素坐标映射](../../src/common/resize/bilinear.cpp#L26-L58)
- [`src/common/resize/bilinear.cpp` 第 60～86 行：四邻域采样与插值](../../src/common/resize/bilinear.cpp#L60-L86)

### 屏幕操作

先停在第 49～58 行，再跳到第 73～86 行。

### 讲稿

“三个后端执行的业务顺序都是中心裁剪、双线性缩放、亮度调整和水印混合。其中双线性缩放是主要逐像素热点。

第 49～58 行使用半像素公式，把输出坐标映射回裁剪区域中的输入坐标。第 60～71 行找到左上、右上、左下、右下四个邻居和两个方向的权重。最后，第 82～86 行先做水平方向插值，再做垂直方向插值，并统一舍入到 8 位像素。

这里最重要的不是公式本身，而是这套语义必须保持唯一。OpenMP 直接复用它；CUDA 在 Device 端实现等价版本。否则测到的就不是同一个计算问题。”

### 过渡

“明确公共像素语义后，先看最简单、也是正确性 Oracle 的 Sequential。”

---

## 5. Sequential：逐图、逐行、逐像素建立基线

### 代码跳转

- [`src/sequential/processor/processor.cpp` 第 11～54 行：串行批处理](../../src/sequential/processor/processor.cpp#L11-L54)
- [`src/sequential/processor/process_image.cpp` 第 8～49 行：缩放与亮度](../../src/sequential/processor/process_image.cpp#L8-L49)
- [`src/sequential/processor/process_image.cpp` 第 51～93 行：水印 Alpha 混合](../../src/sequential/processor/process_image.cpp#L51-L93)

### 屏幕操作

在 `processor.cpp` 展示第 30～37 行，然后切到 `process_image.cpp` 第 24～49 行和第 55～89 行。

### 讲稿

“Sequential 批处理在第 30 行按图片索引逐张执行，没有并行区域。

进入单图函数后，第 24～49 行是三层循环：输出行、输出列和 BGR 通道。每个通道先调用刚才的双线性采样，再应用亮度系数。

基础像素全部完成后，第 51～89 行定位右下角水印区域并进行 Alpha Blending。

因此 Sequential 有两个作用：第一，它是单线程性能基线；第二，它是并行后端的正确性参考。后面的 OpenMP 和 CUDA 可以改变工作分配方式，但不能改变这里定义的结果。”

### 过渡

“OpenMP 的目标，是保留相同 CPU 像素函数，只改变循环怎样分给多个线程。”

---

## 6. OpenMP 第一步：根据图片数选择并行粒度

### 代码跳转

- [`src/benchmark/runner/openmp_executor.cpp` 第 18～40 行：统一执行器适配](../../src/benchmark/runner/openmp_executor.cpp#L18-L40)
- [`src/openmp/scheduling/scheduling.cpp` 第 5～13 行：Images/Rows 选择规则](../../src/openmp/scheduling/scheduling.cpp#L5-L13)
- [`src/openmp/processor/processor.cpp` 第 184～215 行：预检、预分配和策略选择](../../src/openmp/processor/processor.cpp#L184-L215)

### 屏幕操作

先展示 `openmp_executor.cpp` 如何取出线程数，再点击仅 9 行的 `choose_scheduling_strategy()`。

### 讲稿

“统一执行器只负责从实验配置中取出 `thread_count`，然后调用 OpenMP 的 `process_batch`。

OpenMP 后端不是永远使用同一种粒度。这里的选择规则很直接：当线程数大于 1、但图片数少于线程数时，选择 Rows；其他情况选择 Images。

原因是，如果只有一两张图片却只按图片并行，大部分 CPU 线程会空闲。相反，图片数量足够时，按整张图片分配更简单，同步开销也更低。

第 204～209 行先为每张图片预分配独立输出。这个设计让线程写入不同图片或不同输出行，不需要给每个像素加锁。”

### 过渡

“下面分别看图片级动态调度和行级静态调度。”

---

## 7. OpenMP 第二步：图片多时使用 `dynamic, 1`

### 代码跳转

- [`src/openmp/processor/processor.cpp` 第 211～237 行：图片级动态调度](../../src/openmp/processor/processor.cpp#L211-L237)
- [`src/openmp/processor/process_image.cpp` 第 101～116 行：线程内部按单图串行处理](../../src/openmp/processor/process_image.cpp#L101-L116)
- [`src/openmp/processor/processor.cpp` 第 129～179 行：仅进度汇报使用 Mutex](../../src/openmp/processor/processor.cpp#L129-L179)

### 屏幕操作

将光标停在第 219 行的 pragma，再依次指向 `schedule(dynamic, 1)`、`outputs[item]` 和 `tracker.record()`。

### 讲稿

“图片数量足够时，第 219 行使用 `parallel for schedule(dynamic, 1)`。Chunk size 为 1，表示线程每次领取一张图片。

选择 dynamic 是因为输入图片的尺寸和处理成本可能不同。先完成的线程可以继续领取下一张，减少某个慢任务让其他核心等待的负载不均衡。

每次迭代都按固定索引读取 `images[item]`，并只写 `outputs[item]`。任务完成顺序可以变化，但结果位置不会变化，所以输出仍然是确定的。

这里确实有一个 Mutex，但它只保护进度计数和回调，不保护像素。真正的图片计算通过独立输出所有权保持无锁。”

### 过渡

“如果图片数少于线程数，程序改为让线程处理同一张图中的不同输出行。”

---

## 8. OpenMP 第三步：图片少时按行静态调度，并依靠阶段 Barrier

### 代码跳转

- [`src/openmp/processor/processor.cpp` 第 239～257 行：逐图进入 Rows 策略](../../src/openmp/processor/processor.cpp#L239-L257)
- [`src/openmp/processor/process_image.cpp` 第 118～149 行：两个静态行并行区域](../../src/openmp/processor/process_image.cpp#L118-L149)
- [`src/openmp/processor/process_image.cpp` 第 9～36 行：每个线程处理一条输出行](../../src/openmp/processor/process_image.cpp#L9-L36)

### 屏幕操作

展示第 126 行和第 139 行的两个 pragma，并指出第一个并行循环结束的位置。

### 讲稿

“Rows 策略下，图片之间仍然按顺序处理，但每张图内部按输出行并行。

第 126 行先静态分配缩放和亮度阶段的输出行。每一行的计算量接近，所以 static 不需要运行时抢任务，开销更低。

第一个 `parallel for` 结束时存在隐式 Barrier。它保证所有基础像素都已经完成，之后第 139 行才能开始并行水印行。这个同步是正确性要求，因为水印混合必须读取已经完成亮度处理的目标像素。

当前实现没有嵌套并行。程序在图片级和行级之间二选一，避免 Oversubscription，也让线程数量和同步关系更容易解释。”

### 过渡

“OpenMP 共享 CPU 内存；CUDA 则必须先明确 Host 和 Device 之间的数据边界。”

---

## 9. CUDA 公共接口：结果、阶段时间和有效 Batch 分开返回

### 代码跳转

- [`include/parallelpix/cuda/processor.hpp` 第 15～33 行：可用性、阶段时间和处理结果](../../include/parallelpix/cuda/processor.hpp#L15-L33)
- [`include/parallelpix/cuda/processor.hpp` 第 37～55 行：CUDA Processor 公共入口](../../include/parallelpix/cuda/processor.hpp#L37-L55)
- [`src/benchmark/runner/cuda_executor.cpp` 第 31～82 行：M6 到统一 Backend 的适配](../../src/benchmark/runner/cuda_executor.cpp#L31-L82)

### 屏幕操作

依次指出 `phase_timing`、`effective_batch_size` 和 `requested_batch_size`。

### 讲稿

“CUDA API 把三类信息分开返回。

`processing` 保存图片或错误；`phase_timing` 保存 H2D、Kernel 和 D2H 时间；`effective_batch_size` 保存真正执行的批大小。

请求批大小不一定等于实际批大小，因为它可能受 `grid.z` 上限限制，也可能在显存不足后减半。

阶段时间是 optional。失败时不伪造一个看起来有效的性能数字。外层 `CudaExecutor` 只把这些字段适配到三后端共用的 Benchmark 接口。”

### 过渡

“进入 CUDA Processor 后，先检查设备和输入，再决定实际批大小。”

---

## 10. CUDA Host 控制：设备检查、共享预检和 OOM 回退

### 代码跳转

- [`src/cuda/processor/processor.cpp` 第 37～68 行：设备发现与 Executor 初始化](../../src/cuda/processor/processor.cpp#L37-L68)
- [`src/cuda/processor/processor.cpp` 第 70～110 行：输入预检与首选批大小](../../src/cuda/processor/processor.cpp#L70-L110)
- [`src/cuda/processor/processor.cpp` 第 112～150 行：显存不足后减半重试一次](../../src/cuda/processor/processor.cpp#L112-L150)

### 屏幕操作

先指出 `prepare_processing_batch()`，再展示 `first_batch_size` 和 `fallback_batch_size`。

### 讲稿

“CUDA Processor 构造时先查询设备，并只在设备可用时创建 Batch Executor。

处理开始后，第 90 行调用与 Sequential 共用的预检，统一检查图片、水印、输出配置和中心裁剪。随后实际批大小取用户请求值与设备 `max_grid_z` 的较小值。

如果第一次执行抛出 `cudaErrorMemoryAllocation`，并且批大小大于 1，第 123～133 行把 batch 减半，从图片 0 完整重试一次。

从头重试而不是从失败位置续跑，可以避免前一次的阶段时间、进度和部分输出混入最终结果。非显存错误不会盲目重试，而是直接返回明确失败。”

### 过渡

“设备可以执行以后，下一步是把不同尺寸的输入图片变成 GPU 容易处理的连续布局。”

---

## 11. CUDA 数据打包：连续像素缓冲加每图 Descriptor

### 代码跳转

- [`src/cuda/processor/batch_packer.cpp` 第 36～83 行：打包变长输入](../../src/cuda/processor/batch_packer.cpp#L36-L83)
- [`src/cuda/kernels/kernel_contract.hpp` 第 8～18 行：每图 Descriptor](../../src/cuda/kernels/kernel_contract.hpp#L8-L18)
- [`src/cuda/kernels/kernel_contract.hpp` 第 20～29 行：Device 处理配置](../../src/cuda/kernels/kernel_contract.hpp#L20-L29)

### 屏幕操作

展示 `input_offset`，再对照 `packed_input.insert()`。

### 讲稿

“输入商品图可以具有不同宽高，因此不能假设每张图占用相同字节数。

`pack_chunk` 先计算当前 chunk 的总字节数，然后把所有像素顺序追加到一个连续 Host 缓冲。每追加一张图，就生成一个 Descriptor，记录它的起始偏移、宽高、stride 和裁剪区域。

Kernel 先用 `image_index` 找到 Descriptor，再用 `input_offset` 定位该图。这样只需要少量连续 H2D 传输，同时仍然支持变长输入。

输出更简单，因为所有结果都是相同分辨率，所以可以按 `image_index × output_image_bytes` 直接定位。”

### 过渡

“准备好连续数据以后，Batch Executor 按 H2D、Kernel、D2H 的顺序执行每个 chunk。”

---

## 12. CUDA Batch Executor：H2D → Kernel → D2H

### 代码跳转

- [`src/cuda/processor/batch_executor.cpp` 第 44～84 行：资源准备与水印上传](../../src/cuda/processor/batch_executor.cpp#L44-L84)
- [`src/cuda/processor/batch_executor.cpp` 第 86～128 行：分 Chunk、复用缓冲和 H2D](../../src/cuda/processor/batch_executor.cpp#L86-L128)
- [`src/cuda/processor/batch_executor.cpp` 第 130～162 行：Kernel 与 D2H](../../src/cuda/processor/batch_executor.cpp#L130-L162)
- [`src/cuda/processor/batch_executor.cpp` 第 164～196 行：恢复 Image 和返回阶段时间](../../src/cuda/processor/batch_executor.cpp#L164-L196)

### 屏幕操作

沿着第 93、110、136、153、168 行依次向下指。

### 讲稿

“这是 CUDA Host 端最完整的执行顺序。

第 93 行先打包当前 chunk。第 104～108 行调用 `reserve`，让 Device Buffer 在容量足够时直接复用，减少重复 `cudaMalloc`。

第 110 行把图片和 Descriptor 传到 GPU，也就是 H2D。第 136 行启动融合 Kernel。第 153 行再把固定尺寸结果下载回 Host，也就是 D2H。最后，第 164～173 行恢复成项目公共的 `Image` 对象。

三个阶段分别使用 CUDA Event 计时。但是完整 CUDA compute 还包括 Host 打包、结果对象恢复和 API 开销，所以不能只拿 Kernel 时间与 CPU 的完整 compute 比较。”

### 可选补充

“当前实现使用一个非阻塞 Stream，但每个阶段的计时会同步 Event。因此 H2D、Kernel 和 D2H 按顺序执行，没有多 Stream 重叠。”

### 过渡

“现在进入真正的 GPU 并行部分：一个输出像素怎样映射到一个 CUDA 线程。”

---

## 13. CUDA Kernel：二维像素坐标加第三维图片索引

### 代码跳转

- [`src/cuda/kernels/process_batch.cu` 第 173～196 行：16×16 Block 与三维 Grid](../../src/cuda/kernels/process_batch.cu#L173-L196)
- [`src/cuda/kernels/process_batch.cu` 第 89～115 行：线程坐标和输出地址](../../src/cuda/kernels/process_batch.cu#L89-L115)
- [`src/cuda/kernels/process_batch.cu` 第 117～168 行：缩放、亮度和水印融合](../../src/cuda/kernels/process_batch.cu#L117-L168)
- [`src/cuda/kernels/process_batch.cu` 第 39～86 行：Device 双线性采样](../../src/cuda/kernels/process_batch.cu#L39-L86)

### 屏幕操作

建议先讲 Launch，再回到 Kernel：先看第 183～189 行，然后看第 97～101 行，最后看第 129～168 行。

### 讲稿

“Kernel 启动使用 16×16 的二维 Block，也就是每个 Block 有 256 个线程。`grid.x` 和 `grid.y` 通过向上取整覆盖输出平面，`grid.z` 等于当前 chunk 的图片数。

进入 Kernel 后，第 97～101 行计算输出像素的 x、y 和图片索引。也就是说，一个 CUDA 线程负责一张图片中的一个输出像素，并在内部处理三个 BGR 通道。

第 131 行执行与 CPU 等价的半像素双线性采样，第 139 行应用亮度，第 143～166 行只在当前像素位于水印区域时进行 Alpha Blending，最后写入唯一的输出位置。

这称为融合 Kernel，因为 resize、brightness 和 watermark 没有产生三个全局内存中间结果。每个线程只写自己的像素，所以这里不需要 Atomic，也不需要像素锁。线程之间也没有数据依赖，因此不需要 `__syncthreads()`。”

### 可选补充

“逻辑上会启动大量线程，但硬件不会让所有线程在同一时刻运行。GPU 会按 32 线程的 Warp，把 Block 分批调度到 SM 上。”

### 过渡

“并行代码只有在结果正确时才有性能意义，所以最后回到统一验证与计时。”

---

## 14. Benchmark 门禁：验证通过以后才发布 Speedup

### 代码跳转

- [`src/benchmark/runner/experiment.cpp` 第 164～184 行：预热](../../src/benchmark/runner/experiment.cpp#L164-L184)
- [`src/benchmark/runner/experiment.cpp` 第 201～296 行：compute 与 end-to-end 计时域](../../src/benchmark/runner/experiment.cpp#L201-L296)
- [`src/benchmark/runner/experiment.cpp` 第 321～373 行：PNG 回读和后端验证](../../src/benchmark/runner/experiment.cpp#L321-L373)
- [`src/benchmark/runner/experiment.cpp` 第 376～438 行：统计、吞吐量、Speedup 和效率](../../src/benchmark/runner/experiment.cpp#L376-L438)
- [`src/benchmark/runner/experiment.cpp` 第 440～465 行：CUDA 阶段统计与验证失败记录](../../src/benchmark/runner/experiment.cpp#L440-L465)

### 屏幕操作

先展示第 205、241、244、287 行的三组时刻，再跳到第 327～353 行和第 421～438 行。

### 讲稿

“每个实验先执行配置数量指定的 warmup，预热结果不进入正式样本。

正式重复中，第 205～287 行定义了两个计时域。`compute` 只包住后端执行；`end-to-end` 还包含扫描、解码和 PNG 写出。两者必须分开，因为计算加速不代表整条应用链路同倍加速。

写出后，程序重新读取 PNG。Sequential 检查内存结果与编码后结果；OpenMP 和 CUDA 则与同规模 Sequential 输出比较。OpenMP 容差是 0，CUDA 容差上限是每通道 1。

第 421 行是性能门禁：只有 `validation_passed` 为真，才会计算 speedup。OpenMP 再用 speedup 除以线程数得到 parallel efficiency。

所以项目的原则是：快但错误的结果不是有效加速。失败记录可以保留用于诊断，但不会发布一个误导性的性能结论。”

### 过渡

“最后用自动化测试证明，这些正确性约束不是只写在文档里的。”

---

## 15. 测试收尾：OpenMP 要求完全一致，CUDA 允许最大误差 1

### 代码跳转

- [`tests/cpp/openmp/test_processor.cpp` 第 71～119 行：两种 OpenMP 策略与 Sequential 完全一致](../../tests/cpp/openmp/test_processor.cpp#L71-L119)
- [`tests/cpp/cuda/test_processor.cpp` 第 113～149 行：不同 Batch 的 CUDA 像素与阶段时间验证](../../tests/cpp/cuda/test_processor.cpp#L113-L149)
- [`src/cuda/runtime/resources.cpp` 第 121～175 行：DeviceBuffer RAII 与容量复用](../../src/cuda/runtime/resources.cpp#L121-L175)

### 屏幕操作

OpenMP 指向第 87 行的完整像素数组比较；CUDA 指向第 146～148 行的最大误差断言。

### 讲稿

“OpenMP 测试分别覆盖图片级和行级策略，并直接比较宽、高、stride、路径和完整像素数组，因此要求与 Sequential 完全一致。

CUDA 测试覆盖 batch 1、4 和 8，同时验证阶段时间、有效批大小、进度次数和最大像素误差不超过 1。

资源层使用 RAII：Device Buffer 析构时释放显存，`reserve` 只在容量不足时重新分配。这既减少泄漏风险，也避免每个 chunk 重复分配显存。

到这里，整条代码链闭环了：入口建立公平实验，三个后端执行同一语义，Validator 先证明正确，再由 Benchmark 发布性能。”

---

## 结尾总结（约 30 秒）

### 讲稿

“总结来说，ParallelPix 的重点不是简单地把循环加上并行指令。

Sequential 定义正确性和性能基线；OpenMP 在图片级动态调度与行级静态调度之间选择；CUDA 用连续批数据和三维 Grid，把输出像素映射为大量 GPU 线程。

最后，统一 Benchmark 同时记录 compute、end-to-end 和 CUDA 阶段时间，并且只有验证通过才发布 speedup。

因此这个项目把正确性、并行粒度、数据移动和实际性能放在同一条可复现的代码路径里。”

---

## 5 分钟精简路线

时间不足时，只展示以下 7 个位置：

1. [`BenchmarkPlan：自动加入 Sequential 并展开实验`](../../src/planning/benchmark_plan.cpp#L29-L49)
2. [`公共双线性语义`](../../src/common/resize/bilinear.cpp#L49-L86)
3. [`Sequential 三层像素循环`](../../src/sequential/processor/process_image.cpp#L24-L49)
4. [`OpenMP 图片级 dynamic, 1`](../../src/openmp/processor/processor.cpp#L217-L237)
5. [`OpenMP 行级 static 与阶段 Barrier`](../../src/openmp/processor/process_image.cpp#L118-L149)
6. [`CUDA 线程映射与融合 Kernel`](../../src/cuda/kernels/process_batch.cu#L89-L168)
7. [`验证通过后才计算 Speedup`](../../src/benchmark/runner/experiment.cpp#L421-L438)

精简结论：

> 三种后端共享同一处理语义。OpenMP 改变 CPU 循环的任务分配，CUDA 把输出像素映射到 GPU 线程；统一验证通过以后，Benchmark 才计算可发布的加速比。

---

## 现场操作提醒

- 提前按本文顺序打开文件，避免现场在目录树中搜索。
- 每次只高亮当前讲解的 10～30 行，不要整页快速滚动。
- 先说“这一段解决什么问题”，再解释 pragma、线程索引或 API。
- 不要说“所有 GPU 线程同时执行”；应说“逻辑上启动，硬件按 Block/Warp 分批调度”。
- 不要把 Kernel 时间称为 CUDA 总时间；合法比较使用完整 `compute_ms`。
- 不要说 OpenMP 完全不需要同步；像素写入无锁，但阶段依赖需要 Barrier，进度计数需要 Mutex。
- 不要把 CUDA batch size 与 16×16 Block size 混淆。
- 不要把 M8 Hybrid 说成已经实现；当前代码是三个独立后端。
