# ParallelPix

> 基于 OpenMP 与 CUDA 的电商商品图片并行处理课程项目

## 构建与运行

```bash
# C++ CLI 与 Streamlit 仪表板尚未开始实现
# 首个可运行版本完成后补充具体命令
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
