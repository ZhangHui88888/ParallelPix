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
    "image_count": ("Image count", "图片数量"),
    "single_image_count_help": (
        "Run one image count per benchmark so the processing trajectory has one clear X-axis.",
        "每次基准测试只运行一个图片数量，使处理轨迹的 X 轴保持明确。",
    ),
    "openmp_threads": ("OpenMP threads", "OpenMP 线程"),
    "cuda_batch_sizes": ("CUDA batch sizes", "CUDA 批大小"),
    "integer_list_help": (
        "Enter comma-separated positive integers, for example {example}.",
        "请输入以逗号分隔的正整数，例如 {example}。",
    ),
    "measured_repetitions": ("Measured samples", "正式样本数"),
    "measurement_mode": ("Measurement mode", "测量模式"),
    "measurement_steady": ("Steady-state benchmark", "稳态基准"),
    "measurement_cold_start": ("Single-run cold start", "单次冷启动基准"),
    "cold_start_caption": (
        "No warm-up. One fresh CLI process runs every selected configuration once.",
        "无热身；启动一个全新的 CLI 进程，每个已选配置仅运行一次。",
    ),
    "measure_cold_start": ("Measure per-config cold starts", "测量每个配置的冷启动"),
    "measure_cold_start_help": (
        "Runs the selected matrix once in one fresh CLI process without warm-up.",
        "在一个全新的 CLI 进程中无热身运行一次所选矩阵。",
    ),
    "warmup_caption": (
        "Fixed schedule: 2 warm-ups and 5 measured samples for every configuration.",
        "固定计划：每个配置热身 2 次、正式测量 5 次。",
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
    "overview_image_count": ("Compare configurations at", "比较图片数量"),
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
    "cold_start_cli_duration": ("CLI invocation duration", "完整 CLI 调用耗时"),
    "cold_start_baseline_ms": ("Cold-start baseline (ms)", "冷启动基准（毫秒）"),
    "cold_start_cli_note": (
        "Measured outside the executable: process creation through process exit; it includes initialization and this entire benchmark invocation, not a per-configuration value.",
        "在可执行文件外测量：从创建进程到进程退出；包含初始化和本次完整基准调用，不属于单个配置。",
    ),
    "image_sets_short": ("Image sets", "图片集"),
    "image_sets_formula": ("image sets", "个图片集"),
    "sequential_short": ("sequential", "顺序"),
    "threads_short": ("threads", "线程"),
    "batches_short": ("batches", "批次"),
    "reps_short": ("samples", "个样本"),
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
    "chart_cuda_throughput_title": (
        "CUDA throughput by batch size",
        "CUDA 吞吐量与批大小",
    ),
    "no_parallel_overview": (
        "Run OpenMP or CUDA to populate the parallel scaling view.",
        "运行 OpenMP 或 CUDA 后将在此显示并行扩展结果。",
    ),
    "chart_cold_start_title": ("Cold-start duration by configuration", "各配置冷启动耗时"),
    "chart_completed_trajectory": ("Processing trajectory", "处理轨迹"),
    "processed_images": ("Processed images", "已处理图片数"),
    "batch_ms_per_image": ("Batch average (ms / image)", "本批平均（毫秒/张）"),
    "final_average_table": ("Final cumulative averages", "最终累计平均表"),
    "final_comparison": ("Final configuration comparison", "最终配置对比"),
    "final_average_context": (
        "{count} images · final benchmark comparison",
        "{count} 张图片 · 最终基准对比",
    ),
    "chart_final_median": (
        "Median processing time per image — lower is faster",
        "单张处理耗时中位数——越低越快",
    ),
    "chart_final_speedup": ("Final speedup", "最终加速比"),
    "chart_final_throughput": (
        "Final throughput — higher is faster",
        "最终吞吐量——越高越快",
    ),
    "chart_final_total_duration": (
        "End-to-end total duration — lower is faster",
        "端到端总耗时——越低越快",
    ),
    "end_to_end_ms": ("End-to-end total duration (ms)", "端到端总耗时（毫秒）"),
    "end_to_end_seconds": ("End-to-end total duration (s)", "端到端总耗时（秒）"),
    "chart_final_cold_start": ("Cold-start duration — lower is faster", "冷启动耗时——越低越快"),
    "cold_start_cli_ms": ("Cold-start duration (ms)", "冷启动耗时（毫秒）"),
    "cold_start_not_measured": (
        "Per-configuration cold-start data is not available for this run.",
        "本次运行没有逐配置冷启动历史数据。",
    ),
    "median_ms_per_image": ("Median (ms / image)", "中位数（毫秒/张）"),
    "trajectory_not_available": (
        "Run a new benchmark to view its processing trajectory.",
        "重新运行一次基准测试，即可查看处理轨迹。",
    ),
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
    "Image counts must be a comma-separated list of positive 32-bit integers.": "图片数量必须是以逗号分隔的正 32 位整数列表。",
    "OpenMP thread counts must be a comma-separated list of positive 32-bit integers.": "OpenMP 线程数必须是以逗号分隔的正 32 位整数列表。",
    "CUDA batch sizes must be a comma-separated list of positive 32-bit integers.": "CUDA 批大小必须是以逗号分隔的正 32 位整数列表。",
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
