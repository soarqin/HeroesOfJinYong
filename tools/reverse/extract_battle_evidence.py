from __future__ import annotations

import argparse
from pathlib import Path
import sys

try:
    from manifest import (
        BinaryRecord,
        DependencyRecord,
        EvidenceRecord,
        Manifest,
        sha256_file,
    )
except ModuleNotFoundError:
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from manifest import (
        BinaryRecord,
        DependencyRecord,
        EvidenceRecord,
        Manifest,
        sha256_file,
    )


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
    manifest = Manifest(source_root=game_dir.name)
    for record in validate_binaries(game_dir):
        manifest.add_binary(record)
    return manifest


def extract_evidence(game_dir: Path) -> Manifest:
    from ida_backend import IdaDatabase

    manifest = build_validation_manifest(game_dir)
    with IdaDatabase(game_dir / "Z.COM") as zcom:
        zcom.find_exact_string("z.DAT")
        zcom_function = zcom.containing_function(0x10195)
        manifest.add_evidence(EvidenceRecord(
            evidence_id="ENTRY-ZCOM-EXEC",
            binary="Z.COM",
            address="0x10195",
            kind="process-exec",
            summary="Executes Z.DAT through DOS int 21h AX=4B00h",
            status="confirmed",
            function_start=zcom_function["start"],
            function_end=zcom_function["end"],
            function_name=zcom_function["name"],
            pseudocode_summary=tuple(zcom.decompile_summary(0x10195)),
        ))

    manifest.add_dependency(DependencyRecord("Z.COM", "Z.DAT", "process-exec", "0x10195"))
    with IdaDatabase(game_dir / "Z.DAT") as zdat:
        war_sta = zdat.find_exact_string("war.sta")
        war_function = zdat.containing_function(0x31DAC)
        manifest.add_evidence(EvidenceRecord(
            evidence_id="DATA-WAR-LOAD",
            binary="Z.DAT",
            address=f"{war_function['start']}",
            kind="function",
            summary=f"Loads WAR.STA at string address {war_sta:#x}",
            status="confirmed",
            function_start=war_function["start"],
            function_end=war_function["end"],
            function_name=war_function["name"],
            pseudocode_summary=tuple(zdat.decompile_summary(0x31DAC)),
        ))

        fight_string = zdat.find_exact_string("fight000.grp")
        fight_function = zdat.containing_function(0x385F8)
        manifest.add_evidence(EvidenceRecord(
            evidence_id="ANIM-FIGHT-LOAD",
            binary="Z.DAT",
            address=f"{fight_function['start']}",
            kind="function",
            summary=f"Loads FIGHT animation data at string address {fight_string:#x}",
            status="confirmed",
            function_start=fight_function["start"],
            function_end=fight_function["end"],
            function_name=fight_function["name"],
            pseudocode_summary=tuple(zdat.decompile_summary(0x385F8)),
        ))
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

    try:
        extract_evidence(args.game_dir).write(args.output)
    except (ImportError, KeyError, RuntimeError) as error:
        print(f"IDA extraction failed: {error}", file=sys.stderr)
        return 4
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
