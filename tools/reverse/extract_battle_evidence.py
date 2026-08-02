from __future__ import annotations

import argparse
from pathlib import Path
import sys

try:
    from manifest import BinaryRecord, Manifest, sha256_file
except ModuleNotFoundError:
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from manifest import BinaryRecord, Manifest, sha256_file


APPROVED_HASHES = {
    "Z.COM": "9de4c8002f92759bd2771a4984ff02599121e6c66fed1eb0d7e905a327e2dac1",
    "Z.DAT": "0034f836def2b287e62b0902f08cab22b61b397f24d44d6038274f1750a007ce",
}


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Extract reproducible battle reverse-engineering evidence from original DOS binaries."
    )
    parser.add_argument("--game-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--validate-only", action="store_true")
    return parser.parse_args(argv)


def validate_binaries(game_dir: Path) -> list[BinaryRecord]:
    records: list[BinaryRecord] = []
    for name, expected_hash in APPROVED_HASHES.items():
        path = game_dir / name
        if not path.is_file():
            raise FileNotFoundError(f"missing required binary: {name}")
        actual_hash = sha256_file(path)
        if actual_hash != expected_hash:
            raise RuntimeError(
                f"hash mismatch: {name}; expected {expected_hash}, got {actual_hash}"
            )
        records.append(BinaryRecord(name, path.stat().st_size, actual_hash))
    return records


def build_validation_manifest(game_dir: Path) -> Manifest:
    manifest = Manifest(source_root=str(game_dir))
    for record in validate_binaries(game_dir):
        manifest.add_binary(record)
    return manifest


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    try:
        manifest = build_validation_manifest(args.game_dir)
    except FileNotFoundError as error:
        print(str(error), file=sys.stderr)
        return 2
    except RuntimeError as error:
        print(str(error), file=sys.stderr)
        return 3

    if args.validate_only:
        return 0

    print(
        "validated original binaries; full IDA extraction is not available in this step",
        file=sys.stderr,
    )
    manifest.write(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
