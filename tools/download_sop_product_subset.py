"""Download a balanced, product-only subset of Stanford Online Products."""

from __future__ import annotations

import argparse
import csv
import hashlib
import os
import random
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import UTC, datetime
from pathlib import Path
from urllib.request import Request, urlopen


REPOSITORY_ROOT = (
    "https://huggingface.co/datasets/pawlo2013/StanfordOnlineProducts/resolve/main/"
    "Stanford_Online_Products"
)
USER_AGENT = "ParallelPix-course-project/1.0 (local educational dataset preparation)"
CATEGORIES = (
    "bicycle",
    "cabinet",
    "chair",
    "coffee_maker",
    "fan",
    "kettle",
    "lamp",
    "mug",
    "sofa",
    "stapler",
    "table",
    "toaster",
)


def request_bytes(url: str) -> bytes:
    with urlopen(Request(url, headers={"User-Agent": USER_AGENT}), timeout=90) as response:
        return response.read()


def build_selection(count: int) -> list[tuple[str, str]]:
    per_category, remainder = divmod(count, len(CATEGORIES))
    selection: list[tuple[str, str]] = []
    for index, category in enumerate(CATEGORIES):
        names = request_bytes(f"{REPOSITORY_ROOT}/{category}_final.txt").decode("utf-8").splitlines()
        names = [name.strip() for name in names if name.strip()]
        random.Random(20260725 + index).shuffle(names)
        selection.extend((category, name) for name in names[: per_category + (index < remainder)])
    return selection


def download_one(item: tuple[str, str], output: Path) -> dict[str, str]:
    category, original_name = item
    filename = f"{category}__{original_name.lower()}"
    destination = output / filename
    url = f"{REPOSITORY_ROOT}/{category}_final/{original_name}"
    if not destination.exists() or destination.stat().st_size == 0:
        payload = request_bytes(url)
        with tempfile.NamedTemporaryFile(delete=False, dir=output, suffix=".part") as temporary:
            temporary.write(payload)
            temporary_path = Path(temporary.name)
        os.replace(temporary_path, destination)
    return {
        "file": filename,
        "category": category,
        "source_dataset": "Stanford Online Products",
        "source_url": url,
        "usage_note": "Academic product-image dataset; local course testing only; do not redistribute image files.",
        "bytes": str(destination.stat().st_size),
        "sha256": hashlib.sha256(destination.read_bytes()).hexdigest(),
        "downloaded_at_utc": datetime.now(UTC).isoformat(timespec="seconds"),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--count", type=int, default=1000)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--workers", type=int, default=4)
    args = parser.parse_args()
    if args.count < len(CATEGORIES):
        parser.error(f"--count must be at least {len(CATEGORIES)}")

    args.output.mkdir(parents=True, exist_ok=True)
    selection = build_selection(args.count)
    results: list[dict[str, str]] = []
    failures: list[tuple[str, str]] = []
    with ThreadPoolExecutor(max_workers=args.workers) as executor:
        futures = {executor.submit(download_one, item, args.output): item for item in selection}
        for number, future in enumerate(as_completed(futures), start=1):
            category, original_name = futures[future]
            try:
                results.append(future.result())
                print(f"[{number}/{len(selection)}] downloaded {category}/{original_name}", flush=True)
            except Exception as error:  # noqa: BLE001 - keep independent product downloads running.
                failures.append((f"{category}/{original_name}", str(error)))
                print(f"[{number}/{len(selection)}] failed {category}/{original_name}: {error}", flush=True)

    results.sort(key=lambda row: row["file"])
    args.manifest.parent.mkdir(parents=True, exist_ok=True)
    with args.manifest.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(results[0]) if results else ["file"])
        writer.writeheader()
        writer.writerows(results)
    if failures:
        failure_path = args.manifest.with_name(f"{args.manifest.stem}.failures.csv")
        with failure_path.open("w", newline="", encoding="utf-8") as stream:
            writer = csv.writer(stream)
            writer.writerow(["product_path", "error"])
            writer.writerows(failures)
        print(f"Completed {len(results)}/{len(selection)}; failures recorded in {failure_path}", flush=True)
        return 1
    print(f"Completed {len(results)} product images; manifest: {args.manifest}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
