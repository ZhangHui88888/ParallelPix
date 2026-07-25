# M2 CLI Controller 与任务编排测试记录

## 认证要求

无。M2 是本机命令行程序，不提供账号或远程访问。

## 前置条件

- Windows 11。
- Visual Studio 2022 C++ x64 工具链。
- CMake 3.20+。
- Python 3.12 和项目 `.venv` 用于 M1 回归测试。
- 不需要 OpenCV、OpenMP、CUDA、真实图片或外部网络。

## 请求与响应

请求为 `parallelpix benchmark` 命令及 M1 契约参数。响应包括 UTF-8 日志、最终 `[RESULT]` 摘要和进程退出码。M2 单元测试使用替身 Pipeline；真实 CSV 与后端行为由 M7 测试覆盖。

## 执行配置与结果

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
.\build\Debug\parallelpix.exe --help
.\.venv\Scripts\python.exe -m pytest -q
git diff --check
```

## 测试用例

| 场景 | 入参 | 预期 | 实际 | 状态 |
|------|------|------|------|------|
| 帮助命令 | `--help` | 返回 0，输出 Usage，不调用 Pipeline | 与预期一致 | 通过 |
| 缺少参数 | 仅 `benchmark` | 返回 64，输出 CLI 错误与失败摘要 | 与预期一致 | 通过 |
| M1 完整命令 | 默认三后端矩阵 | 参数解析成功，Pipeline 接收 24 项计划 | 与预期一致 | 通过 |
| 数值边界 | 空项、零、负数、文本、溢出 | 返回 64 并指出对应参数 | 与预期一致 | 通过 |
| 后端条件参数 | OpenMP 无 threads、CUDA 无 batches | 返回 64，不调用 Pipeline | 与预期一致 | 通过 |
| 矩阵规范化 | 乱序后端与重复数值 | 自动补 Sequential，规范后端，去重保序 | 与预期一致 | 通过 |
| 全成功摘要 | 24 成功且存在 CSV 路径 | 返回 0 | 与预期一致 | 通过 |
| 部分成功摘要 | 成功、失败和跳过并存 | 返回 2 | 与预期一致 | 通过 |
| 成功但无 CSV | `succeeded > 0`，CSV 为空 | 返回 73 | 与预期一致 | 通过 |
| 摘要矛盾 | 计数和不等于 planned | 返回 70 | 与预期一致 | 通过 |
| Pipeline 异常 | 抛出标准异常 | 捕获、记录并返回 70 | 与预期一致 | 通过 |
| M2 阶段占位进程 | M1 完整命令 | 当时返回 70 且不伪造 CSV | 历史阶段验收；已由 M7 替换 | 通过 |
| UTF-8 进程边界 | 中文命令参数与中文 CSV 路径 | 参数和日志使用合法 UTF-8，无乱码或解码失败 | 与预期一致 | 通过 |
| M1 回归 | Python 测试套件 | 既有仪表板测试全部通过 | 26 passed | 通过 |

## 自动化结果

- C++ 行为测试：27 项。
- CTest：1 个聚合 C++ 测试和 4 个真实进程测试。
- MSVC `/W4 /permissive-` 构建无警告。
- M1 Python 回归：26 项通过。

## 测试目录映射

| M2 子模块 | 测试目录 | 覆盖内容 |
|-----------|----------|----------|
| `cli` | `tests/cpp/cli/` | 命令、参数、数值列表和 UTF-8 路径解析 |
| `planning` | `tests/cpp/planning/` | 后端规范化、去重与实验矩阵展开 |
| `controller` | `tests/cpp/controller/` | Pipeline 调用、日志、摘要校验和退出码 |
| `pipeline` | `tests/cpp/pipeline/` | M7 真实 Pipeline 工厂和后端降级行为 |

`tests/cpp/test_main.cpp` 与 `tests/cpp/test_support.hpp` 是所有 C++ 子模块共享的测试基础设施；`tests/powershell/` 保留真实进程边界测试。
