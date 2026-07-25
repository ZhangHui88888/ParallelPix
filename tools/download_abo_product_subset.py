"""Download high-resolution Amazon Berkeley Objects catalog images for ParallelPix."""

from __future__ import annotations

import argparse
import csv
import gzip
import hashlib
import os
import random
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import UTC, datetime
from pathlib import Path
from urllib.request import Request, urlopen


METADATA_URL = "https://amazon-berkeley-objects.s3.amazonaws.com/images/metadata/images.csv.gz"
ORIGINAL_TEMPLATE = "https://amazon-berkeley-objects.s3.amazonaws.com/images/original/{path}"
USER_AGENT = "ParallelPix-course-project/1.0 (local educational dataset preparation)"


def request(url: str):
    return urlopen(Request(url, headers={"User-Agent": USER_AGENT}), timeout=90)


def selected_images(metadata_file: Path, count: int) -> list[dict[str, str]]:
    with gzip.open(metadata_file, "rt", encoding="utf-8", newline="") as stream:
        candidates = [
            row for row in csv.DictReader(stream)
            if int(row["width"]) >= 1280 and int(row["height"]) >= 1024 and row["path"].lower().endswith((".jpg", ".jpeg"))
        ]
    if len(candidates) < count:
        raise ValueError(f"Only {len(candidates)} images meet the resolution threshold.")
    random.Random(20260725).shuffle(candidates)
    return candidates[:count]


def download_one(item: dict[str, str], output: Path) -> dict[str, str]:
    extension = item["path"].rsplit(".", 1)[1].lower()
    filename = f"abo_{item['image_id'].replace('+', '_')}.{extension}"
    destination = output / filename
    url = ORIGINAL_TEMPLATE.format(path=item["path"])
    if not destination.exists() or destination.stat().st_size == 0:
        with request(url) as response:
            with tempfile.NamedTemporaryFile(delete=False, dir=output, suffix=".part") as temporary:
                temporary.write(response.read())
                temporary_path = Path(temporary.name)
        os.replace(temporary_path, destination)
    return {
        "file": filename, "image_id": item["image_id"], "source_dataset": "Amazon Berkeley Objects (ABO)",
        "source_url": url, "license": "CC BY 4.0", "width": item["width"], "height": item["height"],
        "bytes": str(destination.stat().st_size), "sha256": hashlib.sha256(destination.read_bytes()).hexdigest(),
        "downloaded_at_utc": datetime.now(UTC).isoformat(timespec="seconds"),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--count", type=int, default=1000)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--metadata", type=Path, required=True)
    parser.add_argument("--workers", type=int, default=4)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    selection = selected_images(args.metadata, args.count)
    rows, failures = [], []
    with ThreadPoolExecutor(max_workers=args.workers) as executor:
        futures = {executor.submit(download_one, item, args.output): item for item in selection}
        for number, future in enumerate(as_completed(futures), start=1):
            item = futures[future]
            try:
                rows.append(future.result())
                print(f"[{number}/{len(selection)}] downloaded", flush=True)
            except Exception as error:  # noqa: BLE001
                failures.append((item["image_id"], str(error)))
                print(f"[{number}/{len(selection)}] failed: {error}", flush=True)
    rows.sort(key=lambda row: row["file"])
    with args.manifest.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]) if rows else ["file"])
        writer.writeheader()
        writer.writerows(rows)
    if failures:
        with args.manifest.with_name(f"{args.manifest.stem}.failures.csv").open("w", newline="", encoding="utf-8") as stream:
            writer = csv.writer(stream)
            writer.writerow(["image_id", "error"])
            writer.writerows(failures)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
