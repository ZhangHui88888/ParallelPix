# M2 CLI Controller 与任务编排实施计划

## 目标

使用 C++17 标准库实现独立、可测试的 CLI 控制层，覆盖参数解析、校验、实验矩阵构建、下游 Pipeline 端口、日志和退出码。M2 不实现图片解码、像素算法、性能统计或 CSV 写入。

## 全局约束

- 首要目标环境为 Windows 11、Visual Studio 2022、CMake 3.20+。
- M2 不依赖 OpenCV、OpenMP、CUDA 或第三方 CLI/测试框架。
- `warmups` 固定为 2，`repetitions` 至少为 5。
- 路径存在性、权限和图片解码交给 M3。
- 未链接 M3～M7 时不得创建 CSV 或返回成功。

## 实施任务

| 任务 | 交付物 | 验证 | 状态 |
|------|--------|------|------|
| C++17/CMake 骨架 | `parallelpix_m2`、`parallelpix`、`parallelpix_m2_tests` | MSVC Debug 构建 | 已完成 |
| CLI 解析与校验 | M1 参数契约、帮助、错误报告 | 表驱动 C++ 测试 | 已完成 |
| BenchmarkPlan 构建 | 后端规范化、去重保序、矩阵展开 | 默认矩阵 24 项 | 已完成 |
| Controller 与 Pipeline 端口 | 调用隔离、摘要校验、日志与退出码 | Fake Pipeline 测试 | 已完成 |
| 可执行入口 | 薄 `main`、可替换 Pipeline 工厂 | 真实进程 CTest | 已完成；M7 已替换占位实现 |
| 文档与回归 | 设计、测试记录、README 和索引 | CTest、pytest、diff check | 已完成 |

## 完成定义

- `parallelpix --help` 返回 0。
- 参数错误返回 64，且不调用 Pipeline。
- 合法请求只调用一次 Pipeline，并传递规范化的 `BenchmarkPlan`。
- 全成功返回 0，部分成功返回 2，完全失败按类别返回 65、69、70 或 73。
- M2 阶段的占位构建曾对合法请求返回 70；M7 现已通过同一端口提供真实成功与部分成功路径。
- C++ 构建、CTest、M1 Python 回归和 `git diff --check` 全部通过。
