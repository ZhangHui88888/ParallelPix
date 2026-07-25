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
