# M3 图片数据 I/O 测试记录

## 认证要求

无。M3 是本机 C++ 图片 I/O 库，不提供账号、网络接口或远程访问。

## 前置条件

- Windows 11、Visual Studio 2022 C++ x64 工具链。
- CMake 3.20+ 和 vcpkg manifest mode。
- 固定 baseline 的 OpenCV 4.12.0，启用 JPEG 与 PNG。
- Python 3.12 与 Pillow 仅用于生成已提交的小型确定性夹具。

## 请求与响应

请求为本地目录、图片或水印路径、批次大小，以及完整 PNG 目标路径和显式写入策略。响应为带可选数据和 `IoIssue` 列表的结构化结果；M3 不直接打印日志，不生成 CSV。

## 测试夹具

- 3×2 彩色 PNG、4×3 JPEG、2×2 灰度 PNG。
- 中文文件名 PNG。
- 2×2 RGBA、2×1 RGB、1×2 灰度水印。
- 扩展名合法但内容损坏的图片与水印。

夹具由 `tests/fixtures/generate_m3_fixtures.py` 确定性生成；C++ 测试只读取已生成文件，不依赖 Python。

## 执行配置

```powershell
$env:VCPKG_ROOT = "<vcpkg-root>"
cmake -S . -B build/m3 -G "Visual Studio 17 2022" -A x64 `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build/m3 --config Debug
ctest --test-dir build/m3 -C Debug --output-on-failure
cmake --build build/m3 --config Release
ctest --test-dir build/m3 -C Release --output-on-failure
.\.venv\Scripts\python.exe -m pytest -q
git diff --check
```

## 测试用例

| 场景 | 入参 | 预期 | 实际 | 状态 |
|------|------|------|------|------|
| 路径校验 | 缺失目录、普通文件、空目录 | 返回对应结构化错误 | 与预期一致 | 通过 |
| 扫描排序 | 混合扩展名、子目录和非图片文件 | 只返回第一层支持文件并稳定排序 | 与预期一致 | 通过 |
| 图片解码 | 彩色 PNG、JPEG、灰度、中文路径 | 连续三通道 BGR，尺寸与来源正确 | 与预期一致 | 通过 |
| 水印解码 | RGBA、RGB、灰度、损坏文件 | Alpha 保留或补 255，损坏文件失败 | 与预期一致 | 通过 |
| 坏图补足 | 坏图后跟多个有效图片 | 警告坏图并补足指定数量 | 与预期一致 | 通过 |
| 数量不足 | 请求量大于有效图片数 | 整批失败，不返回缩小批次 | 与预期一致 | 通过 |
| 输出准备 | 新目录和路径为文件 | 创建目录并清理探针；无效路径失败 | 与预期一致 | 通过 |
| PNG 写出 | 正常、冲突、替换、目录同名 | 像素级往返；严格遵守写入策略 | 与预期一致 | 通过 |
| M2 边界（M3 阶段） | 合法 Benchmark 请求 | 当时返回 70 且不生成 CSV | 历史阶段验收；已由 M7 替换 | 通过 |
| M1 回归 | Python 测试套件 | 既有仪表板行为全部通过 | 26 项通过 | 通过 |

## 自动化结果

- M3 C++ 行为测试：10 项，Debug 与 Release 全部通过。
- M2 C++ 行为测试：27 项，Debug 与 Release 全部通过。
- CTest：2 个聚合 C++ 测试和 4 个真实进程测试，Debug/Release 均 6/6 通过。
- M1 Python 回归：26 项通过。
- MSVC `/W4 /permissive- /utf-8`：Debug/Release 构建无警告。
- `git diff --check`：通过。
