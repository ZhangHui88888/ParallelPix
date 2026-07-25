# ParallelPix

> 基于 OpenMP 与 CUDA 的电商商品图片并行处理课程项目

## 构建与运行

### M2 C++ CLI

在 Visual Studio 2022 Developer PowerShell 中执行：

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\parallelpix.exe --help
```

当前可执行文件只包含 M2 Controller。它可以完整解析和校验 M1 命令、构建实验矩阵并输出稳定日志和退出码；M3～M7 尚未链接，因此合法 Benchmark 请求会返回 70，不生成或伪造 CSV。

### M1 本地仪表板

M1 仪表板使用 Python 3.12。首次运行：

```powershell
py -3.12 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
.\.venv\Scripts\python.exe -m streamlit run tools/dashboard.py
```

默认进入明确标记的 Demo 模式，不需要已编译的 C++ CLI。切换到 `Local CLI` 后，页面会按 [M1 设计契约](docs/design/M1本地性能仪表板设计.md) 启动 M2。

Windows 下也可以直接双击根目录的 `start_dashboard.bat`：脚本会创建或复用 `.venv`、修复失效的 Python 3.12 虚拟环境、安装固定版本依赖并启动页面。只检查和修复运行环境而不启动页面时，可执行：

```powershell
.\start_dashboard.bat --check-only
```

运行自动化测试：

```powershell
.\.venv\Scripts\python.exe -m pip install -r requirements-dev.txt
.\.venv\Scripts\python.exe -m pytest -q
```

## 架构概览

- **技术栈**：C++17、OpenMP、CUDA、OpenCV、CMake、Python、Streamlit
- **架构模式**：本地 Streamlit 仪表板 + C++ CLI Controller + 可替换计算后端
- **核心模块**：M1 本地仪表板、M2 CLI Controller、M3 图片 I/O、M4 Sequential、M5 OpenMP、M6 CUDA、M7 验证与 Benchmark
- **详细设计**：见 [`docs/tech/技术架构文档.md`](docs/tech/技术架构文档.md)

## 目录结构与模块映射

### 先看结论

- **M1 本地性能仪表板**：全部 Python 页面代码在 `tools/parallelpix_dashboard/`；启动入口是 `tools/dashboard.py`。
- **M2 CLI Controller**：按 `cli`、`planning`、`controller`、`pipeline` 四个子模块拆分；实现位于 `src/`，公共接口位于 `include/parallelpix/`。
- **M3～M7**：目录骨架已就位，但尚未实现、也尚未接入 CMake；`output/`、`results/` 和 `data/` 仍只保存本地数据或产物，不包含模块实现。

### 当前实际目录

下面的树状图只展示已存在的、与开发相关的目录和文件；`.venv/`、`build/`、`.pytest_cache/`、`.tmp/` 等本机生成目录不纳入项目结构。

```text
ParallelPix/
├── CMakeLists.txt                  # C++17 构建、M2 可执行程序和 C++ 测试注册
├── requirements.txt                # M1 运行依赖
├── requirements-dev.txt            # M1 测试依赖
├── start_dashboard.bat             # Windows 下创建环境并启动 M1
├── include/
│   └── parallelpix/                # M2 公共 C++ 接口
│       ├── cli/cli.hpp             # 命令行请求、后端和 CSV 模型
│       ├── planning/benchmark_plan.hpp # 实验计划与单项实验模型
│       ├── controller/controller.hpp # Controller、日志与退出码接口
│       ├── pipeline/pipeline_factory.hpp # Pipeline 创建接口
│       ├── common/                  # M3～M6 共用的图片、配置与处理接口（待实现）
│       ├── io/                      # M3 图片读取、校验和写出接口（待实现）
│       ├── sequential/              # M4 单线程基线接口（待实现）
│       ├── openmp/                  # M5 OpenMP 后端接口（待实现）
│       ├── cuda/                    # M6 CUDA 后端接口（待实现）
│       └── benchmark/               # M7 验证、统计和报告接口（待实现）
├── src/
│   ├── cli/
│   │   ├── main.cpp                # 可执行程序入口与 UTF-8 参数转换
│   │   └── cli_parser.cpp          # 命令行解析与语义校验
│   ├── planning/benchmark_plan.cpp # 由请求生成实验矩阵
│   ├── controller/controller.cpp   # 编排、日志和退出码
│   ├── pipeline/controller_only_pipeline.cpp # M3～M7 未接入时的占位 Pipeline
│   ├── common/{geometry,resize,effects}/ # M3～M6 共享像素语义（待实现）
│   ├── io/{catalog,decode,encode}/ # M3 扫描、解码和编码（待实现）
│   ├── sequential/processor/       # M4 单线程处理流水线（待实现）
│   ├── openmp/{scheduling,processor}/ # M5 调度和并行处理（待实现）
│   ├── cuda/{runtime,transfer,kernels}/ # M6 运行时、传输和 Kernel（待实现）
│   └── benchmark/{runner,validation,reporting,statistics}/ # M7 基准执行、校验、报告和统计（待实现）
├── tools/
│   ├── dashboard.py                # M1 Streamlit 启动入口
│   ├── download_*.py               # 下载公开商品图片样本的辅助脚本
│   └── parallelpix_dashboard/      # M1 仪表板实现
│       ├── page.py                 # 页面组装、状态初始化和运行流程
│       ├── sidebar.py              # 参数表单与运行按钮
│       ├── models.py               # 请求、运行模式和运行状态模型
│       ├── validation.py           # 页面参数校验
│       ├── runners.py              # Demo / Local CLI 执行器
│       ├── results.py              # Benchmark CSV 读取与校验
│       ├── charts.py、views.py     # 图表与结果视图
│       ├── components.py           # 可复用页面组件
│       ├── i18n.py                 # 中英文案
│       ├── styles.py               # Streamlit 样式
│       ├── preview.py              # 图片预览
│       └── assets/demo_results.csv # Demo 模式的示例结果
├── tests/
│   ├── cpp/
│   │   ├── cli/                    # M2 CLI Parser 测试
│   │   ├── planning/               # M2 实验矩阵测试
│   │   ├── controller/             # M2 编排与退出码测试
│   │   ├── pipeline/               # M2 Pipeline 工厂测试
│   │   ├── common/                  # 共享像素语义测试（待实现）
│   │   ├── io/                      # M3 I/O 测试（待实现）
│   │   ├── sequential/              # M4 基线测试（待实现）
│   │   ├── openmp/                  # M5 并行一致性与线程数测试（待实现）
│   │   ├── cuda/                    # M6 Kernel、传输与设备降级测试（待实现）
│   │   ├── benchmark/               # M7 验证、指标和 CSV 测试（待实现）
│   │   └── test_main.cpp、test_support.hpp # 共享 C++ 测试入口与断言
│   ├── powershell/assert_cli_process.ps1 # M2 真实进程断言
│   ├── fixtures/fake_cli.py        # M1 调用 CLI 的测试替身
│   ├── conftest.py                 # Python 测试公共夹具
│   └── test_*.py                   # M1 模型、校验、结果、Runner、样式和启动测试
├── data/                           # 输入数据：图片清单、样本目录和水印
├── output/                         # 本地处理图片输出；不提交生成内容
├── results/                        # 本地 Benchmark CSV 输出；不提交生成内容
└── docs/                           # 需求、设计、计划、技术说明和测试记录
```

### 模块状态与代码位置

| 模块 | 当前实现位置 | 对应测试 | 状态 |
|------|--------------|----------|------|
| M1 本地性能仪表板 | `tools/dashboard.py`、`tools/parallelpix_dashboard/` | `tests/test_dashboard_app.py`、`tests/test_*.py` | 已实现；支持 Demo 与 Local CLI 模式 |
| M2 CLI Controller 与任务编排 | `src/{cli,planning,controller,pipeline}/`、`include/parallelpix/{cli,planning,controller,pipeline}/` | `tests/cpp/{cli,planning,controller,pipeline}/`、`tests/powershell/` | 已实现；负责解析、校验、计划、日志和退出码 |
| M3 图片数据 I/O | `src/io/{catalog,decode,encode}/`、`include/parallelpix/io/` | `tests/cpp/io/`、`tests/fixtures/{images,watermarks}/` | 目录已就位；待实现、未接入构建 |
| M4 Sequential 基线 | `src/sequential/processor/`、`include/parallelpix/sequential/` | `tests/cpp/sequential/` | 目录已就位；待实现、未接入构建 |
| M5 OpenMP 后端 | `src/openmp/{scheduling,processor}/`、`include/parallelpix/openmp/` | `tests/cpp/openmp/` | 目录已就位；待实现、未接入构建 |
| M6 CUDA 后端 | `src/cuda/{runtime,transfer,kernels}/`、`include/parallelpix/cuda/` | `tests/cpp/cuda/` | 目录已就位；待实现、未接入构建 |
| M7 验证与 Benchmark | `src/benchmark/{runner,validation,reporting,statistics}/`、`include/parallelpix/benchmark/` | `tests/cpp/benchmark/` | 目录已就位；待实现、未接入构建 |

### 后续模块的落位约定

实现 M3～M7 时必须在既有目录中落位，不在 `src/cli/`、`src/controller/` 或模块外目录插入业务实现。共享像素语义放在 `common/`；后端仅调用公共模型并实现各自的计算策略。每个模块首次落地时，才将其源文件、依赖与测试加入 `CMakeLists.txt`，并同步更新本节、相关 `docs/` 设计和测试记录。

## 编码约定

- **Git**：不在 `main`/`master` 直接工作，使用 `feature/描述-MMDD` 分支
- **文档**：项目文档统一维护在 `docs/`
- **核心算法**：OpenCV 只用于解码、编码和内存容器，像素处理由项目自行实现
- **测试**：按 M1～M7 模块维护正确性、异常和性能测试记录

## 外部依赖

| 依赖 | 用途 |
|------|------|
| Visual Studio 2022、CMake | 构建和测试 C++17 M2 CLI |
| OpenCV | 图片解码、编码和内存容器 |
| OpenMP | 多核 CPU 并行处理 |
| CUDA Toolkit | GPU 并行处理与计时 |
| Streamlit、Pandas、Plotly | 本地性能仪表板与图表 |

## 文档导航

- **开发计划** → [`docs/plan/`](docs/plan/)
- **需求与设计** → [`docs/design/`](docs/design/)
- **技术文档** → [`docs/tech/`](docs/tech/)
- **测试记录** → [`docs/test/`](docs/test/)
完整文档索引见 [`docs/README.md`](docs/README.md)
