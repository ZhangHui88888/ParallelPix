"""Download a reproducible Open Images subset from an ImageID CSV file."""

from __future__ import annotations

import argparse
import csv
import hashlib
import os
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import UTC, datetime
from pathlib import Path
from urllib.request import Request, urlopen


BASE_URL = "https://open-images-dataset.s3.amazonaws.com/validation/{image_id}.jpg"
USER_AGENT = "ParallelPix-course-project/1.0 (local educational dataset preparation)"


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--input", type=Path, required=True, help="CSV containing ImageID and Labels columns")
    parser.add_argument("--output", type=Path, required=True, help="Directory for original JPEG files")
    parser.add_argument("--manifest", type=Path, required=True, help="Manifest CSV to create")
    parser.add_argument("--workers", type=int, default=4, help="Concurrent downloads (default: 4)")
    return parser.parse_args()


def download_one(item: dict[str, str], output: Path) -> dict[str, str]:
    image_id = item["ImageID"]
    destination = output / f"{image_id}.jpg"
    url = BASE_URL.format(image_id=image_id)

    if not destination.exists() or destination.stat().st_size == 0:
        request = Request(url, headers={"User-Agent": USER_AGENT})
        with urlopen(request, timeout=90) as response:
            with tempfile.NamedTemporaryFile(delete=False, dir=output, suffix=".part") as temporary:
                temporary.write(response.read())
                temporary_path = Path(temporary.name)
        os.replace(temporary_path, destination)

    digest = hashlib.sha256(destination.read_bytes()).hexdigest()
    return {
        "file": destination.name,
        "image_id": image_id,
        "labels": item["Labels"],
        "source_url": url,
        "license": "Open Images V5 image metadata: CC BY 2.0 (verify per-image before redistribution)",
        "bytes": str(destination.stat().st_size),
        "sha256": digest,
        "downloaded_at_utc": datetime.now(UTC).isoformat(timespec="seconds"),
    }


def main() -> int:
    args = parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    with args.input.open(newline="", encoding="utf-8-sig") as stream:
        items = list(csv.DictReader(stream))

    results: list[dict[str, str]] = []
    failures: list[tuple[str, str]] = []
    with ThreadPoolExecutor(max_workers=args.workers) as executor:
        futures = {executor.submit(download_one, item, args.output): item["ImageID"] for item in items}
        for number, future in enumerate(as_completed(futures), start=1):
            image_id = futures[future]
            try:
                results.append(future.result())
                print(f"[{number}/{len(items)}] downloaded {image_id}", flush=True)
            except Exception as error:  # noqa: BLE001 - continue downloading independent files.
                failures.append((image_id, str(error)))
                print(f"[{number}/{len(items)}] failed {image_id}: {error}", flush=True)

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
            writer.writerow(["image_id", "error"])
            writer.writerows(failures)
        print(f"Completed {len(results)}/{len(items)}; failures recorded in {failure_path}", flush=True)
        return 1

    print(f"Completed {len(results)} images; manifest: {args.manifest}", flush=True)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
