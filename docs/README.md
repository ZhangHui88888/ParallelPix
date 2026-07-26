# 文档目录说明

> ParallelPix — 基于 OpenMP 与 CUDA 的电商商品图片并行处理课程项目

## 目录结构

```
docs/
├── README.md
├── plan/      # 项目计划
├── design/    # 需求、功能设计与交互流程
├── tech/      # 技术架构与本地构建运行说明
└── test/      # 正确性、异常与性能实验记录
```

## `plan/`

| 文档 | 说明 |
|------|------|
| [开发计划.md](./plan/开发计划.md) | 项目开发进度总览、里程碑与模块任务 |
| [M1仪表板界面美化实施计划.md](./plan/M1仪表板界面美化实施计划.md) | M1 数据优先布局、中英切换、自动化和视觉验证实施步骤 |
| [M2 CLI Controller与任务编排实施计划.md](./plan/M2%20CLI%20Controller与任务编排实施计划.md) | M2 C++17 控制层、测试与验收实施步骤 |
| [M3图片数据IO实施计划.md](./plan/M3图片数据IO实施计划.md) | M3 共享图片模型、扫描、解码、输出与验收步骤 |
| [M4公共处理模型与Sequential实施计划.md](./plan/M4公共处理模型与Sequential实施计划.md) | M4 公共像素语义、单线程后端、集成与验收步骤 |
| [M6 CUDA后端实施计划.md](./plan/M6%20CUDA后端实施计划.md) | M6 条件构建、GPU 批处理、故障回退、联调与验收步骤 |
| [M7验证与Benchmark实施计划.md](./plan/M7验证与Benchmark实施计划.md) | M7 验证、计时、统计、真实 Pipeline 与报告实施步骤 |
| [M8 CPU-GPU混合后端实施计划.md](./plan/M8%20CPU-GPU混合后端实施计划.md) | M8 异构任务划分、并发执行、轨迹、Schema 扩展与性能实验步骤 |

## `design/`

| 文档 | 说明 |
|------|------|
| [项目需求文档.md](./design/项目需求文档.md) | 为什么做、做什么：背景、目标、角色、范围与验收标准 |
| [功能设计文档.md](./design/功能设计文档.md) | 如何使用和运转：模块、页面、流程、规则、权限与异常状态 |
| [M1本地性能仪表板设计.md](./design/M1本地性能仪表板设计.md) | M1 页面、Runner、状态流及 M2/M7 数据契约 |
| [M1本地性能仪表板界面美化设计.md](./design/M1本地性能仪表板界面美化设计.md) | M1 数据优先实验工作台的布局、视觉系统、状态与验收标准 |
| [M2 CLI Controller与任务编排设计.md](./design/M2%20CLI%20Controller与任务编排设计.md) | M2 CLI、实验矩阵、Pipeline、日志及退出码设计 |

| [M3图片数据IO设计.md](./design/M3图片数据IO设计.md) | M3 图片模型、目录扫描、解码、批次与 PNG 输出契约 |
| [M4公共处理模型与Sequential设计.md](./design/M4公共处理模型与Sequential设计.md) | M4 配置、裁剪、缩放、效果、错误模型与 Sequential 契约 |
| [M6 CUDA后端设计.md](./design/M6%20CUDA后端设计.md) | M6 构建模式、数据布局、融合 Kernel、计时和显存回退契约 |
| [M7验证与Benchmark设计.md](./design/M7验证与Benchmark设计.md) | M7 执行器、测量、验证、输出、CSV 和降级契约 |
| [M8 CPU-GPU混合后端设计.md](./design/M8%20CPU-GPU混合后端设计.md) | M8 OpenMP/CUDA 同批并发、任务比例、合并验证与指标契约 |

ParallelPix 不包含对外 API 或数据库，因此不维护 API/数据库设计文档；CLI、CSV 和构建契约统一记录在模块设计与技术架构文档中。

## `tech/`

| 文档 | 说明 |
|------|------|
| [技术架构文档.md](./tech/技术架构文档.md) | 技术选型、分层架构、核心依赖 |

ParallelPix 不包含对外 API、数据库或生产部署。构建与本地运行说明维护在根目录 [README.md](../README.md)。

## `test/`

记录单元、集成、正确性、异常和性能实验，详见 [test/README.md](./test/README.md)。

| 文档 | 说明 |
|------|------|
| [M1本地性能仪表板测试.md](./test/M1本地性能仪表板测试.md) | M1 自动化、异常与浏览器视觉测试记录 |
| [M2 CLI Controller与任务编排测试.md](./test/M2%20CLI%20Controller与任务编排测试.md) | M2 参数、矩阵、Controller 与真实进程测试记录 |
| [M3图片数据IO测试.md](./test/M3图片数据IO测试.md) | M3 目录、解码、水印、批次和 PNG 输出测试记录 |
| [M4公共处理模型与Sequential测试.md](./test/M4公共处理模型与Sequential测试.md) | M4 几何、像素语义、Sequential 和 M3 集成测试记录 |
| [M6 CUDA后端测试.md](./test/M6%20CUDA后端测试.md) | M6 CPU-only/真机 CUDA、故障、Sanitizer、CLI、性能和仪表板测试记录 |
| [M7验证与Benchmark测试.md](./test/M7验证与Benchmark测试.md) | M7 统计、验证、报告、Runner 和真实进程测试记录 |
