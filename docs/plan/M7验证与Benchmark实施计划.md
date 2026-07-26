# M7 验证与 Benchmark 实施计划

> 状态补充（2026-07-26）：M7 Core 已由 M6 注册 CUDA 执行器；本文中“不实现 CUDA”描述的是 M7 Core 当时的模块边界，不再代表当前项目状态。

## 目标

完成 FR-007～FR-009 的公共基础设施，并先以 Sequential 打通 M2→M3→M4→M7→CSV→M1 的真实链路。M5/M6 后续通过同一执行器契约接入，不修改 CLI 或 CSV。

## 任务状态

| 任务 | 交付物 | 状态 |
|---|---|---|
| 公共契约 | 后端执行器、验证、统计和 27 列记录模型 | 已完成 |
| 统计与验证 | 中位数、总体标准差、结构与像素容差比较 | 已完成 |
| 原子报告 | Overwrite、Append、Unicode 和表头保护 | 已完成 |
| Benchmark Runner | 预检、2 次预热、N 次测量、输出与聚合 | 已完成 |
| Sequential 真实 Pipeline | M2 工厂接线、PNG、CSV、退出状态 | 已完成 |
| 后端降级 | OpenMP/CUDA 未注册时跳过并返回部分成功 | 已完成 |
| 测试与文档 | 单元、集成、真实进程、M1 Schema 和文档闭环 | 已完成 |
| M5/M6 接入 | 注册执行器并启用跨后端验证、阶段计时 | M6 已完成；等待 M5 |
| M8 接入 | Hybrid 执行器、CPU 比例和向后兼容的28列 Schema | 等待 M5；见独立 M8 计划 |

## 实施边界

- M2 继续负责参数、计划、日志和退出码。
- M3 继续负责目录、解码和 PNG；M7 只调用公开 I/O。
- M4 Sequential 像素语义不变。
- 不实现 OpenMP/CUDA，不写伪造结果。
- 不增加 CSV 列，不让 M1 计算性能指标。

上述“不增加 CSV 列”是 M7 Core/M5/M6 阶段的冻结边界。M8 属于后续接口版本，只有在用户确认并完成旧27列兼容方案后，才按 [M8 实施计划](./M8%20CPU-GPU混合后端实施计划.md) 追加 `hybrid_cpu_share`。

## 验收标准

- Sequential 请求执行全部预热和正式重复，输出 PNG 与一行聚合 CSV。
- 混合请求只运行已注册后端，返回部分成功并生成新的 `run_id`。
- 验证失败行保留原始计时但不发布加速比。
- Append/Overwrite 均不会在失败时破坏既有 CSV。
- Unicode 输入、输出、水印和 CSV 路径可通过真实进程运行。
- Debug/Release CTest、M1 Python 回归和 `git diff --check` 全部通过。
