# 本地测试资源

此目录只保存运行时需要的图片资源；大批量图片、生成输出和性能结果不提交到 Git。

```text
data/
├── watermark.png       # 小型透明水印，可提交
├── images/             # CLI 与仪表板的默认输入目录；本地图片，已忽略
├── samples/            # 30～50 张演示图片；已忽略
├── benchmark/          # 100～5000 张性能数据集；已忽略
├── manifest.csv        # 可提交的来源、尺寸和校验值清单（后续创建）
└── manifest.local.csv  # 仅本机的下载/扩展记录；已忽略
```

## 使用约定

- `data/images/`：当前命令行与仪表板默认读取的位置。调试时可放入小型样本集。
- `data/samples/`：用于本地 Demo 的真实商品图片；建议 JPEG，30～50 张。
- `data/benchmark/`：正式性能输入。推荐主测试集为 1000 张约 1080p JPEG（约 1～2 GB），不纳入版本控制。
- `tests/fixtures/images/`：仅放可提交的极小、可控 PNG，用于自动化正确性测试；不要把真实商品图放进这里。
- `output/` 与 `results/`：运行生成的图片、CSV 和图表，均为本机产物。

将大图片放入上述目录前，请确认其来源和使用许可；在 `manifest.csv` 中记录文件名、来源 URL、下载日期、尺寸、格式与 SHA-256。

## 当前主测试集

`benchmark/sop-products-1000/` 已下载 1025 张 Stanford Online Products（SOP）原始 JPEG（约 27.13 MB）。该集的图片主体是电商商品，覆盖 bicycle、cabinet、chair、coffee_maker、fan、kettle、lamp、mug、sofa、stapler、table 和 toaster 12 个商品类别；不使用“画面中碰巧出现某物体”的通用场景标注。

图片由 `tools/download_sop_product_subset.py` 按类别均衡下载；对应的本地清单为 `benchmark/sop-products-1000-manifest.csv`，包含商品类别、来源 URL、大小与 SHA-256。该集仅用于本地课程测试，不重新分发图片文件。

该集用于端到端、正确性和规模实验。若后续 CUDA 性能测试需要更高的像素负载，应另建并记录高分辨率子集，而不是伪造或放大现有基准数据。
