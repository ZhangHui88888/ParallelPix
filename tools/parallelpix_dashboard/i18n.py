from __future__ import annotations

from typing import Literal


Language = Literal["en", "zh"]


COPY: dict[str, tuple[str, str]] = {
    "app_title": ("ParallelPix", "ParallelPix"),
    "app_subtitle": (
        "Local benchmark console for image-processing performance",
        "本地图片处理性能基准控制台",
    ),
    "language": ("Language", "语言"),
    "configuration": ("Configuration", "配置"),
    "experiment_matrix": ("Experiment matrix", "实验矩阵"),
    "run_mode": ("Run mode", "运行模式"),
    "run_mode_help": (
        "Demo uses bundled fictional data. Local CLI launches the compiled M2 executable.",
        "演示模式使用内置虚构数据；本地 CLI 模式会启动已编译的 M2 可执行文件。",
    ),
    "mode_demo": ("Demo", "演示"),
    "mode_local_cli": ("Local CLI", "本地 CLI"),
    "cli_executable": ("CLI executable", "CLI 可执行文件"),
    "input_directory": ("Input directory", "输入目录"),
    "output_directory": ("Output directory", "输出目录"),
    "watermark_image": ("Watermark image", "水印图像"),
    "result_csv": ("Results CSV", "结果 CSV"),
    "backends": ("Backends", "后端"),
    "baseline_info": (
        "Sequential will be added as the comparison baseline.",
        "将自动加入 Sequential 作为对比基线。",
    ),
    "image_counts": ("Image counts", "图片数量"),
    "openmp_threads": ("OpenMP threads", "OpenMP 线程"),
    "cuda_batch_sizes": ("CUDA batch sizes", "CUDA 批大小"),
    "measured_repetitions": ("Measured repetitions", "测量重复次数"),
    "warmup_caption": (
        "Warm-up runs are fixed at 2 for comparable results.",
        "为保证结果可比，热身运行固定为 2 次。",
    ),
    "run_benchmark": ("Run benchmark", "运行基准测试"),
    "backend_sequential": ("Sequential", "Sequential"),
    "backend_openmp": ("OpenMP", "OpenMP"),
    "backend_cuda": ("CUDA", "CUDA"),
    "demo_badge": ("DEMO MODE", "演示模式"),
    "demo_notice": (
        "Results are for demonstration only.",
        "结果仅用于界面演示。",
    ),
    "local_badge": ("LOCAL CLI", "本地 CLI"),
    "local_notice": ("Runs the compiled benchmark locally.", "在本机运行已编译的基准程序。"),
    "demo_banner": (
        "Demo data — not measured. No image processing or CLI execution will occur.",
        "演示数据——未经测量；不会执行图像处理或 CLI 命令。",
    ),
    "displayed_demo_banner": (
        "Displayed results are Demo data — not measured. Run Local CLI to replace them.",
        "当前显示的是未经测量的演示数据；运行本地 CLI 后可替换这些结果。",
    ),
    "status": ("Status", "状态"),
    "status_idle": ("Idle", "空闲"),
    "status_validating": ("Validating", "正在校验"),
    "status_running": ("Running", "正在运行"),
    "status_success": ("Completed", "已完成"),
    "status_partial": ("Partial", "部分完成"),
    "status_failed": ("Failed", "失败"),
    "idle_message": (
        "Configure and run a benchmark to see results.",
        "配置并运行基准测试以查看结果。",
    ),
    "request_validation_failed": ("Request validation failed", "请求校验失败"),
    "benchmark_running": ("Benchmark is running", "基准测试正在运行"),
    "results_invalid": ("Benchmark results are invalid", "基准测试结果无效"),
    "benchmark_completed": ("Benchmark completed", "基准测试已完成"),
    "benchmark_partial": ("Benchmark completed with partial results", "基准测试部分完成"),
    "benchmark_failed": ("Benchmark failed", "基准测试失败"),
    "partial_warning": (
        "Some requested configurations were skipped or failed.",
        "部分请求的配置被跳过或执行失败。",
    ),
    "matrix_backends": ("Backends", "后端"),
    "matrix_image_sets": ("Image sets", "图片组"),
    "matrix_cpu_threads": ("CPU threads (OpenMP)", "CPU 线程（OpenMP）"),
    "matrix_cuda_batches": ("CUDA batch sizes", "CUDA 批大小"),
    "matrix_counts": ("{count} sets", "{count} 组"),
    "selected_count": ("{count} selected", "已选 {count} 项"),
    "config_count": ("{count} configs", "{count} 组配置"),
    "not_selected": ("Not selected", "未选择"),
    "overview": ("Overview", "概览"),
    "scalability": ("Scalability", "可扩展性"),
    "cuda_timing": ("CUDA Timing", "CUDA 时序"),
    "raw_data": ("Raw Data", "原始数据"),
    "empty_results_title": ("No benchmark results yet", "暂无基准测试结果"),
    "empty_results_body": (
        "Run the benchmark to populate charts and tables.",
        "运行基准测试后将在此显示图表和表格。",
    ),
    "benchmark_run": ("Benchmark run", "基准运行"),
    "validated_configurations": ("Validated configs", "已验证配置"),
    "best_speedup": ("Best speedup", "最佳加速比"),
    "peak_throughput": ("Peak throughput", "峰值吞吐量"),
    "fastest_compute": ("Fastest compute", "最快计算时间"),
    "invalid_configurations": (
        "{count} configuration(s) failed validation and are excluded from charts.",
        "{count} 组配置未通过验证，已从图表中排除。",
    ),
    "read_only_source": ("Read-only source", "只读数据源"),
    "no_validated_overview": (
        "No validated rows are available for overview charts.",
        "没有可用于概览图表的已验证数据。",
    ),
    "no_openmp_rows": (
        "No validated OpenMP rows are available.",
        "没有可用的已验证 OpenMP 数据。",
    ),
    "no_cuda_rows": (
        "No validated CUDA timing rows are available.",
        "没有可用的已验证 CUDA 时序数据。",
    ),
    "download_selected_run": ("Download selected run", "下载所选运行"),
    "run_context": ("Run context", "运行上下文"),
    "context_experiment_matrix": ("Experiment matrix", "实验矩阵"),
    "context_run_status": ("Run status", "运行状态"),
    "total_measurements": ("Total measurements", "总测量次数"),
    "result_rows": ("Result rows", "结果行数"),
    "result_source": ("CSV", "CSV"),
    "duration": ("Duration", "耗时"),
    "image_sets_short": ("Image sets", "图片集"),
    "image_sets_formula": ("image sets", "个图片集"),
    "sequential_short": ("sequential", "顺序"),
    "threads_short": ("threads", "线程"),
    "batches_short": ("batches", "批次"),
    "reps_short": ("reps", "次重复"),
    "mode": ("Mode", "模式"),
    "repetitions": ("Repetitions", "重复次数"),
    "run_id": ("Run ID", "运行 ID"),
    "last_run": ("Last run", "最近运行"),
    "no_run": ("—", "—"),
    "chart_compute_title": ("Median compute time", "计算时间中位数"),
    "chart_speedup_title": ("Speedup by configuration", "各配置加速比"),
    "chart_throughput_title": ("Throughput by configuration", "各配置吞吐量"),
    "chart_scalability_title": ("OpenMP scalability", "OpenMP 可扩展性"),
    "chart_cuda_title": ("CUDA timing breakdown", "CUDA 时序分解"),
    "images": ("Images", "图片数"),
    "median_compute_ms": ("Median compute time (ms)", "计算时间中位数（毫秒）"),
    "speedup": ("Speedup", "加速比"),
    "images_per_second": ("Images / second", "图片/秒"),
    "threads": ("OpenMP threads", "OpenMP 线程"),
    "batch_size": ("Batch size", "批大小"),
    "host_to_device": ("Host to device", "主机到设备"),
    "kernel": ("Kernel", "Kernel"),
    "device_to_host": ("Device to host", "设备到主机"),
}


EXACT_MESSAGES_ZH = {
    "Configure a benchmark matrix and start a run.": "配置基准测试矩阵并开始运行。",
    "Benchmark request validation failed.": "基准测试请求校验失败。",
    "Demo benchmark completed with bundled sample data.": "已使用内置样例数据完成演示基准测试。",
    "Benchmark CLI could not be started.": "无法启动基准测试 CLI。",
    "Benchmark finished but did not create the result CSV.": "基准测试已结束，但未创建结果 CSV。",
    "Benchmark finished but did not append a new run_id.": "基准测试已结束，但未追加新的 run_id。",
    "Benchmark completed successfully.": "基准测试已成功完成。",
    "Benchmark completed with partial results.": "基准测试已完成，但仅获得部分结果。",
    "Select at least one processing backend.": "请至少选择一个处理后端。",
    "Select at least one image count.": "请至少选择一个图片数量。",
    "Select at least one OpenMP thread count.": "请至少选择一个 OpenMP 线程数。",
    "Select at least one CUDA batch size.": "请至少选择一个 CUDA 批大小。",
    "M1 requires exactly 2 warm-up runs.": "M1 要求热身运行恰好为 2 次。",
    "Benchmark repetitions must be at least 5.": "基准测试至少需要重复 5 次。",
    "The result path must use the .csv extension.": "结果路径必须使用 .csv 扩展名。",
    "Result CSV is empty.": "结果 CSV 为空。",
    "Result CSV contains no benchmark rows.": "结果 CSV 不包含基准测试记录。",
    "CSV contains an empty run_id.": "CSV 中存在空 run_id。",
    "CSV contains an invalid recorded_at_utc value.": "CSV 中存在无效的 recorded_at_utc 值。",
}

PREFIX_MESSAGES_ZH = {
    "CLI executable not found: ": "未找到 CLI 可执行文件：",
    "Input directory not found: ": "未找到输入目录：",
    "Watermark file not found: ": "未找到水印文件：",
    "Output path is not a directory: ": "输出路径不是目录：",
    "Demo result fixture not found: ": "未找到演示结果文件：",
    "Benchmark CLI failed with exit code ": "基准测试 CLI 失败，退出码：",
    "Result CSV not found: ": "未找到结果 CSV：",
    "Missing required CSV columns: ": "结果 CSV 缺少必需列：",
    "Invalid validation_passed values: ": "validation_passed 包含无效值：",
    "Unable to read result CSV: ": "无法读取结果 CSV：",
}


def tr(key: str, language: Language, **values: object) -> str:
    """Return formatted Streamlit-owned copy in the requested language."""
    index = 0 if language == "en" else 1
    return COPY[key][index].format(**values)


def localize_message(message: str, language: Language) -> str:
    """Translate known application messages while preserving raw technical output."""
    if language == "en":
        return message
    if message in EXACT_MESSAGES_ZH:
        return EXACT_MESSAGES_ZH[message]
    for prefix, translated_prefix in PREFIX_MESSAGES_ZH.items():
        if message.startswith(prefix):
            return f"{translated_prefix}{message.removeprefix(prefix)}"
    return message
