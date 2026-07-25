# M2 CLI Controller 与任务编排设计

## 目标与边界

M2 是本地命令行入口和 Application Controller。它把 M1 或终端参数转换为确定性的实验计划，通过单一 Pipeline 端口交给后续模块，并把结果映射为日志和进程退出码。

M2 不检查真实文件是否存在，不解码图片，不执行 Sequential/OpenMP/CUDA 像素计算，不计算性能指标，也不写 Benchmark CSV。

## 数据流

```text
argv
  → CLI 解析与语义校验
  → BenchmarkRequest
  → 后端规范化与矩阵展开
  → BenchmarkPlan
  → IBenchmarkPipeline
  → WorkflowSummary
  → 日志与退出码
```

无效请求和帮助请求不会调用 Pipeline。后续 M3～M7 通过 `IBenchmarkPipeline` 的实现接入，不需要修改 M1 命令、CLI Parser 或 Controller。

## 源码目录边界

M2 按职责拆分，目录名与公开头文件、测试目录一一对应：

| 子模块 | 实现 | 公共接口 | 测试 | 职责 |
|--------|------|----------|------|------|
| `cli` | `src/cli/` | `include/parallelpix/cli/` | `tests/cpp/cli/` | 进程入口、UTF-8 参数转换、命令解析与语义校验 |
| `planning` | `src/planning/` | `include/parallelpix/planning/` | `tests/cpp/planning/` | 后端规范化和实验矩阵展开 |
| `controller` | `src/controller/` | `include/parallelpix/controller/` | `tests/cpp/controller/` | 调用 Pipeline、写日志、校验摘要并映射退出码 |
| `pipeline` | `src/pipeline/` | `include/parallelpix/pipeline/` | `tests/cpp/pipeline/` | 创建当前 Pipeline；M7 已提供真实实现 |

跨子模块只能依赖公开头文件；`main.cpp` 只保留进程入口与参数编码转换，不承载解析、计划或编排逻辑。

## CLI 契约

```text
parallelpix benchmark
  --input <dir> --output <dir> --watermark <file>
  --backends sequential,openmp,cuda
  --image-counts 10,50,100
  --threads 1,2,4,8
  --cuda-batches 1,4,8
  --warmups 2 --repetitions 5
  --csv <file> --append
```

- 未知、重复、缺值参数均拒绝。
- 数值列表允许项目内空白，拒绝空项、零、负数、非数字和 32 位无符号整数溢出。
- OpenMP 需要 `--threads`，CUDA 需要 `--cuda-batches`；未选择对应后端时允许 M1 继续传入这些列表。
- `--append` 表示追加 CSV；未传入时使用覆盖模式。
- M2 只验证路径字符串和 `.csv` 后缀，实际文件系统检查属于 M3。

## 实验矩阵

- 选择 OpenMP 或 CUDA 时自动加入 Sequential 基线。
- 后端固定按 Sequential、OpenMP、CUDA 排列。
- 图片数量、线程数和 CUDA 批大小去重并保留首次出现顺序。
- Sequential：每个图片数量一项。
- OpenMP：图片数量与线程数的笛卡尔积。
- CUDA：图片数量与 CUDA 批大小的笛卡尔积。
- 线程数与 CUDA 批大小不互相交叉。

M1 默认配置生成 3 个 Sequential、12 个 OpenMP 和 9 个 CUDA 实验，共 24 项。

## Pipeline 与结果

`IBenchmarkPipeline::execute(const BenchmarkPlan&, LogSink)` 返回 `WorkflowSummary`，包含计划、成功、失败、跳过数量、CSV 路径、问题列表和主失败类别。

Controller 验证：

- `planned` 必须等于计划实验数；
- `succeeded + failed + skipped` 必须等于 `planned`；
- 只要存在成功项，就必须提供结果 CSV 路径；
- 摘要矛盾、未捕获异常或未知失败统一视为内部失败。

M2 初始阶段使用 `ControllerOnlyPipeline` 防止伪造成功。M7 已将具体工厂拆入 `parallelpix_benchmark`：Sequential 可真实运行，未注册的 OpenMP/CUDA 配置返回跳过；M2 的 `IBenchmarkPipeline` 和 Controller 校验规则保持不变。

## 日志与退出码

日志格式：

```text
[LEVEL][stage] message
[RESULT] status=<success|partial|failed> code=<number> ...
```

Windows 入口使用宽字符参数并显式转换为 UTF-8，文件系统路径通过 `u8path()` 构造、通过 `u8string()` 写入日志，保证 M1 可以固定使用 UTF-8 解码包含中文的参数、路径和错误消息。

| 退出码 | 含义 |
|--------|------|
| 0 | 全部配置成功并提供 CSV |
| 2 | 至少一项成功，且存在失败或跳过 |
| 64 | 命令或参数错误 |
| 65 | 输入准备失败 |
| 69 | 请求的后端不可用且无成功结果 |
| 70 | 处理失败、摘要矛盾或内部异常 |
| 73 | 结果 CSV 输出失败 |

## 验收标准

- M1 命令契约可被完整解析。
- 默认实验矩阵严格为 24 项。
- 无效输入不调用 Pipeline；有效输入只调用一次。
- 日志为 UTF-8 文本，M1 无需解析即可直接展示。
- M2 核心测试不依赖具体后端；M7 真实进程测试验证 CSV 与成功/部分成功结果。
