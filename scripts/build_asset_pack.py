#!/usr/bin/env python3

import argparse
import hashlib
import json
import shutil
from pathlib import Path


IGNORED_NAMES = {".DS_Store"}
PACK_NAME = "Choroboros Assets"
SCHEMA_VERSION = 1
RELEASE_ASSET_PATHS = {
    "green/green_light_off_backpanel.png",
    "green/green_light_on_backpanel.png",
    "green/rate/green_1_off.png",
    "green/rate/green_1_on.png",
    "green/depth/green_2_off.png",
    "green/depth/green_2_on.png",
    "green/offset/green_3_off.png",
    "green/offset/green_3_on.png",
    "green/width/green_4_off.png",
    "green/width/green_4_on.png",
    "green/green_mix_knob_spritesheet.png",
    "green/green_slider_thumb.png",
    "blue/blue_light_off_backpanel.png",
    "blue/blue_light_on_backpanel.png",
    "blue/rate/blue_1_off.png",
    "blue/rate/blue_1_on.png",
    "blue/depth/blue_2_off.png",
    "blue/depth/blue_2_on.png",
    "blue/offset/blue_3_off.png",
    "blue/offset/blue_3_on.png",
    "blue/width/blue_4_off.png",
    "blue/width/blue_4_on.png",
    "blue/blue_mix_knob_spritesheet.png",
    "blue/blue_slider_thumb.png",
    "red/red_light_off_backpanel.png",
    "red/red_light_on_backpanel.png",
    "red/rate/red_1_off.png",
    "red/rate/red_1_on.png",
    "red/depth/red_2_off.png",
    "red/depth/red_2_on.png",
    "red/offset/red_3_off.png",
    "red/offset/red_3_on.png",
    "red/width/red_4_off.png",
    "red/width/red_4_on.png",
    "red/red_mix_knob_spritesheet.png",
    "red/red_slider_thumb.png",
    "purple/purple_light_off_backpanel.png",
    "purple/purple_light_on_backpanel.png",
    "purple/rate/purple_1_off.png",
    "purple/rate/purple_1_on.png",
    "purple/depth/purple_2_off.png",
    "purple/depth/purple_2_on.png",
    "purple/offset/purple_3_off.png",
    "purple/offset/purple_3_on.png",
    "purple/width/purple_4_off.png",
    "purple/width/purple_4_on.png",
    "purple/purple_mix_knob_spritesheet.png",
    "purple/purple_slider_thumb.png",
    "black/black_light_off_backpanel.png",
    "black/black_light_on_backpanel.png",
    "black/rate/black_1_off.png",
    "black/rate/black_1_on.png",
    "black/depth/black_2_off.png",
    "black/depth/black_2_on.png",
    "black/offset/black_3_off.png",
    "black/offset/black_3_on.png",
    "black/width/black_4_off.png",
    "black/width/black_4_on.png",
    "black/black_mix_knob_spritesheet.png",
    "black/black__slider_thumb.png",
    "switch_a_spritesheet.png",
    "gui_icons/lock.svg",
    "gui_icons/unlock.svg",
    "gui_icons/about.png",
    "gui_icons/dev.png",
    "gui_icons/help.png",
    "gui_icons/bug_feedback_button.png",
    "gui_icons/kaizen_logo.png",
    "fonts/Technology.ttf",
    "fonts/Retroica.ttf",
    "fonts/labels/JetBrainsMonoNL-Thin.ttf",
    "fonts/labels/JetBrainsMono-ThinItalic.ttf",
    "fonts/titles/JetBrainsMono-ExtraBold.ttf",
    "fonts/tooltips/JetBrainsMonoNL-ExtraLight.ttf",
    "fonts/tooltips/JetBrainsMonoNL-ExtraLightItalic.ttf",
}


def sha256_for_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def iter_asset_files(source_root: Path):
    for path in sorted(source_root.rglob("*")):
        if not path.is_file():
            continue
        if path.name in IGNORED_NAMES:
            continue
        relative_path = path.relative_to(source_root).as_posix()
        if relative_path not in RELEASE_ASSET_PATHS:
            continue
        yield path


def write_size_report(report_path: Path, entries, total_bytes: int) -> None:
    top_entries = sorted(entries, key=lambda entry: entry["sizeBytes"], reverse=True)[:20]
    lines = [
        f"Pack name: {PACK_NAME}",
        f"Asset count: {len(entries)}",
        f"Total bytes: {total_bytes}",
        "",
        "Top 20 largest assets:",
    ]

    for entry in top_entries:
        lines.append(f'- {entry["relativePath"]}: {entry["sizeBytes"]}')

    report_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Build a versioned Choroboros shared asset pack.")
    parser.add_argument("--source-root", required=True, help="Source Assets directory")
    parser.add_argument("--output-root", required=True, help="Directory that will contain the versioned asset pack")
    parser.add_argument("--pack-version", required=True, help="Asset pack version")
    args = parser.parse_args()

    source_root = Path(args.source_root).resolve()
    output_root = Path(args.output_root).resolve()
    pack_version = args.pack_version.strip()

    if not source_root.is_dir():
        raise SystemExit(f"Source root does not exist: {source_root}")

    pack_root = output_root / f"ChoroborosAssets-{pack_version}"
    if pack_root.exists():
        shutil.rmtree(pack_root)
    pack_root.mkdir(parents=True, exist_ok=True)

    entries = []
    total_bytes = 0

    for source_file in iter_asset_files(source_root):
        relative_path = source_file.relative_to(source_root).as_posix()
        destination_file = pack_root / relative_path
        destination_file.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source_file, destination_file)

        size_bytes = destination_file.stat().st_size
        total_bytes += size_bytes
        entries.append(
            {
                "id": relative_path,
                "relativePath": relative_path,
                "sizeBytes": size_bytes,
                "sha256": sha256_for_file(destination_file),
            }
        )

    manifest = {
        "schemaVersion": SCHEMA_VERSION,
        "packName": PACK_NAME,
        "assetPackVersion": pack_version,
        "assetCount": len(entries),
        "totalBytes": total_bytes,
        "assets": entries,
    }

    (pack_root / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    write_size_report(pack_root / "size-report.txt", entries, total_bytes)
    print(str(pack_root))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
