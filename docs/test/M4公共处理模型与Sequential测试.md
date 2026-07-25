# M4 公共处理模型与 Sequential 测试记录

## 认证要求

无。M4 是本机 C++ 图片处理库，不提供账号、网络接口或远程访问。

## 前置条件

- Windows 11、Visual Studio 2022 C++ x64 工具链。
- CMake 3.20+ 和 vcpkg manifest mode。
- OpenCV 4.12.0 仅由集成测试中的 M3 解码和 PNG 写出使用。
- Python 3.12 虚拟环境用于 M1 回归测试。

## 请求与响应

请求为 `Image` 批次、`Watermark` 和 `ProcessingConfig`。成功响应包含完整输出批次；失败响应包含 `ProcessingIssue` 列表且没有输出图片。M4 不直接打印日志、命名输出文件或生成 CSV。

## 执行配置

```powershell
$env:VCPKG_ROOT = "<vcpkg-root>"
cmake -S . -B build/m4 -G "Visual Studio 17 2022" -A x64 `
  "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build/m4 --config Debug
ctest --test-dir build/m4 -C Debug --output-on-failure
cmake --build build/m4 --config Release
ctest --test-dir build/m4 -C Release --output-on-failure
.\.venv\Scripts\python.exe -m pytest -q
git diff --check
```

## 测试用例

| 场景 | 入参 | 预期 | 实际 | 状态 |
|------|------|------|------|------|
| 配置校验 | 零尺寸、非法亮度/透明度、极端尺寸 | 返回 `InvalidConfig` 或配置无效 | 与预期一致 | 通过 |
| 中心裁剪 | 方形、横图、竖图、奇数差值、1×1、uint32 边界 | 最大等比例中心区域且无溢出 | 与预期一致 | 通过 |
| 双线性采样 | 2×2 BGR 输入、单位和 1×1 输出 | 半像素采样、边缘钳制、通道正确 | 与预期一致 | 通过 |
| 亮度与限幅 | 普通值、半值舍入、接近 255 | 最近整数舍入并限幅 | 与预期一致 | 通过 |
| 水印混合 | Alpha 0/64/128/255 | 与全局透明度组合后的像素符合公式 | 与预期一致 | 通过 |
| Sequential 正常流程 | 多图片、不同来源路径 | 顺序、路径、像素与输出尺寸正确 | 与预期一致 | 通过 |
| 裁剪与完整效果 | 横图、亮度和右下水印 | 固定处理顺序且非覆盖区域不变 | 与预期一致 | 通过 |
| 确定性与输入不变 | 同一请求重复两次 | 输出逐像素一致，输入未修改 | 与预期一致 | 通过 |
| 整批失败 | 空批次、坏图、坏水印、过大水印 | 无部分输出并返回对应错误 | 与预期一致 | 通过 |
| Unicode 集成 | 中文输入/输出路径 | M3 解码→M4→PNG→回读逐像素一致 | 与预期一致 | 通过 |
| M2 边界（M4 阶段） | 合法 Benchmark 请求 | 当时占位返回 70，不生成 CSV | 历史阶段验收；已由 M7 替换 | 通过 |

## 自动化结果

- M4 C++ 行为测试：12 项，Debug 与 Release 全部通过。
- CTest：M2、M3、M4 聚合测试和 4 个真实进程测试，Debug/Release 均 7/7 通过。
- M1 Python 回归：26 项通过。
- MSVC `/W4 /permissive- /utf-8`：Debug/Release 构建无警告。
- `git diff --check`：通过。
