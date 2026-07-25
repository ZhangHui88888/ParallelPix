"""Download high-resolution files from Wikimedia Commons' Product photography category."""

from __future__ import annotations

import argparse
import csv
import hashlib
import os
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from datetime import UTC, datetime
from pathlib import Path
from urllib.parse import urlencode
from urllib.request import Request, urlopen


API = "https://commons.wikimedia.org/w/api.php"
CATEGORY = "Category:Product photography"
USER_AGENT = "ParallelPix-course-project/1.0 (local educational dataset preparation)"


def request_json(parameters: dict[str, str]) -> dict:
    url = f"{API}?{urlencode(parameters)}"
    with urlopen(Request(url, headers={"User-Agent": USER_AGENT}), timeout=90) as response:
        import json

        return json.load(response)


def all_category_files() -> list[str]:
    titles: list[str] = []
    continuation: dict[str, str] = {}
    while True:
        payload = request_json({
            "action": "query", "list": "categorymembers", "cmtitle": CATEGORY,
            "cmtype": "file", "cmlimit": "500", "format": "json", **continuation,
        })
        titles.extend(member["title"] for member in payload["query"]["categorymembers"])
        if "continue" not in payload:
            return titles
        continuation = {"cmcontinue": payload["continue"]["cmcontinue"]}


def qualified_files(min_width: int, min_height: int) -> list[dict[str, str]]:
    candidates: list[dict[str, str]] = []
    titles = all_category_files()
    for start in range(0, len(titles), 40):
        payload = request_json({
            "action": "query", "prop": "imageinfo", "titles": "|".join(titles[start : start + 40]),
            "iiprop": "url|size|mime|extmetadata", "iiurlwidth": "1600", "format": "json",
        })
        for page in payload["query"]["pages"].values():
            info = page.get("imageinfo", [{}])[0]
            if info.get("mime") != "image/jpeg" or info.get("width", 0) < min_width or info.get("height", 0) < min_height:
                continue
            metadata = info.get("extmetadata", {})
            candidates.append({
                "title": page["title"], "url": info["url"], "download_url": info.get("thumburl", info["url"]),
                "source_page": info["descriptionurl"],
                "license": metadata.get("LicenseShortName", {}).get("value", "unspecified"),
                "width": str(info["width"]), "height": str(info["height"]),
            })
    return candidates


def download_one(item: dict[str, str], output: Path) -> dict[str, str]:
    stem = hashlib.sha256(item["url"].encode("utf-8")).hexdigest()[:16]
    destination = output / f"commons_product_{stem}.jpg"
    if not destination.exists() or destination.stat().st_size == 0:
        with urlopen(Request(item["download_url"], headers={"User-Agent": USER_AGENT}), timeout=90) as response:
            with tempfile.NamedTemporaryFile(delete=False, dir=output, suffix=".part") as temporary:
                temporary.write(response.read())
                temporary_path = Path(temporary.name)
        os.replace(temporary_path, destination)
    return {
        "file": destination.name, "title": item["title"], "source_page": item["source_page"],
        "original_url": item["url"], "download_url": item["download_url"], "license": item["license"], "width": item["width"],
        "height": item["height"], "bytes": str(destination.stat().st_size),
        "sha256": hashlib.sha256(destination.read_bytes()).hexdigest(),
        "downloaded_at_utc": datetime.now(UTC).isoformat(timespec="seconds"),
    }


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--min-width", type=int, default=1280)
    parser.add_argument("--min-height", type=int, default=1024)
    parser.add_argument("--workers", type=int, default=1)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    candidates = qualified_files(args.min_width, args.min_height)
    if not candidates:
        raise SystemExit("No qualified product photographs found.")

    rows, failures = [], []
    with ThreadPoolExecutor(max_workers=args.workers) as executor:
        futures = {executor.submit(download_one, item, args.output): item for item in candidates}
        for number, future in enumerate(as_completed(futures), start=1):
            item = futures[future]
            try:
                rows.append(future.result())
                print(f"[{number}/{len(candidates)}] downloaded", flush=True)
            except Exception as error:  # noqa: BLE001
                failures.append((item["title"], str(error)))
                print(f"[{number}/{len(candidates)}] failed: {error}", flush=True)

    rows.sort(key=lambda row: row["file"])
    with args.manifest.open("w", newline="", encoding="utf-8") as stream:
        writer = csv.DictWriter(stream, fieldnames=list(rows[0]) if rows else ["file"])
        writer.writeheader()
        writer.writerows(rows)
    if failures:
        with args.manifest.with_name(f"{args.manifest.stem}.failures.csv").open("w", newline="", encoding="utf-8") as stream:
            writer = csv.writer(stream)
            writer.writerow(["title", "error"])
            writer.writerows(failures)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
