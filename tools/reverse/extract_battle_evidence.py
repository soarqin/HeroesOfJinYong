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
            pseudocode_summary=(),
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
            pseudocode_summary=(),
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
            pseudocode_summary=(),
        ))
        battle_anchors = [
            ("BATTLE-MAIN-LOOP", 0x31EB9, "battle-loop",
             "Initializes combatants and dispatches turn updates through the battle loop."),
            ("BATTLE-AI-SELECT", 0x33599, "ai-decision",
             "Chooses item, rest, skill and movement branches using health, MP and random thresholds."),
            ("BATTLE-AI-HP-RECOVERY", 0x33C4D, "ai-hp-recovery",
             "Chooses self-healing, an HP item or action code 8 for an eligible ally provider."),
            ("BATTLE-AI-DEPOISON-RECOVERY", 0x33E93, "ai-depoison-recovery",
             "Chooses self-depoison, an item or action code 9 for an eligible ally provider."),
            ("BATTLE-AI-MEDIC-TARGET", 0x341F6, "ai-medic-target",
             "Selects the first eligible ally for medical support using slot order and health thresholds."),
            ("BATTLE-AI-DEPOISON-TARGET", 0x343DA, "ai-depoison-target",
             "Selects the first eligible ally for depoison support using slot order and poison thresholds."),
            ("BATTLE-AI-FOLLOWUP", 0x34550, "ai-followup",
             "Chooses ally support, throwing items or learned skills after resource checks."),
            ("BATTLE-AI-RANDOM-SKILL", 0x34C47, "ai-random-skill",
             "Counts available skills, selects a random slot, then routes through target checks, movement, skill execution or fallback."),
            ("BATTLE-AI-RETREAT", 0x34AEC, "ai-retreat",
             "Scans map cells for the candidate farthest from opposing units, moves there when found and can continue follow-up handling."),
            ("BATTLE-AI-TARGET-STRATEGY", 0x3505B, "ai-target-strategy",
             "Uses three threshold-gated random branches for target selection, then falls back to the default target routine."),
            ("BATTLE-ACT-DAMAGE", 0x3598C, "damage-state",
             "Applies damage, poison, HP and hurt changes for a selected target."),
            ("BATTLE-AI-MEDIC-ACTION", 0x36210, "ai-medic-action",
             "Moves toward the selected ally until medical range is reached, then performs the action."),
            ("BATTLE-AI-DEPOISON-ACTION", 0x363AC, "ai-depoison-action",
             "Moves toward the selected ally until depoison range is reached, then performs the action."),
            ("BATTLE-AI-MOVE-TO-RANGE", 0x3650E, "ai-move-to-range",
             "Checks range, searches candidate tiles for targeted modes, otherwise steps toward the destination and emits movement when a passable tile changes."),
            ("BATTLE-SKILL-SELECT", 0x37734, "skill-selection",
             "Selects a learned skill and validates range, target and movement constraints."),
            ("BATTLE-SKILL-DAMAGE", 0x39188, "skill-damage",
             "Computes skill damage, hurt growth and poison growth using original random bounds."),
            ("BATTLE-ACT-POISON", 0x39A45, "action-poison",
             "Computes a capped poison increment from attacker and target stats, writes the target poison field and returns the applied amount."),
            ("BATTLE-ACT-DEPOISON", 0x39DA3, "action-depoison",
             "Computes a random bounded poison reduction, cancels it above the ability threshold, clamps it and subtracts it from the target."),
            ("BATTLE-ACT-MEDIC", 0x3A10C, "action-medic",
             "Requires stamina of at least 50, computes a random heal, caps target HP, reduces hurt and consumes stamina."),
            ("BATTLE-ACT-THROW", 0x3A30B, "action-throw",
             "Checks throw path and range, sets facing, applies item effects to an enemy target, updates statuses and consumes the thrown item."),
            ("BATTLE-ACT-REST", 0x3A8A4, "action-rest",
             "Marks a rest action, restores stamina with random gain and restores HP and MP with caps after the stamina threshold is reached."),
            ("BATTLE-ROUND-END", 0x3C563, "round-end-status",
             "Applies poison and hurt damage after the complete actor loop."),
            ("BATTLE-RANDOM-MOD", 0x3D612, "random-modulo",
             "Returns the original modulo random value for bounds from 2 through 30000; other bounds return zero without consuming randomness."),
        ]
        for evidence_id, address, kind, summary in battle_anchors:
            function = zdat.containing_function(address)
            manifest.add_evidence(EvidenceRecord(
                evidence_id=evidence_id,
                binary="Z.DAT",
                address=f"{address:#x}",
                kind=kind,
                summary=summary,
                status="confirmed",
                function_start=function["start"],
                function_end=function["end"],
                function_name=function["name"],
                pseudocode_summary=(),
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
