from __future__ import annotations

from dataclasses import asdict, dataclass, field
import hashlib
import json
from pathlib import Path


@dataclass(frozen=True)
class BinaryRecord:
    path: str
    size: int
    sha256: str


@dataclass(frozen=True)
class DependencyRecord:
    parent: str
    child: str
    kind: str
    address: str


@dataclass(frozen=True)
class EvidenceRecord:
    evidence_id: str
    binary: str
    address: str
    kind: str
    summary: str
    status: str
    function_start: str | None = None
    function_end: str | None = None
    function_name: str | None = None
    pseudocode_summary: tuple[str, ...] = ()


@dataclass
class Manifest:
    source_root: str
    binaries: list[BinaryRecord] = field(default_factory=list)
    dependencies: list[DependencyRecord] = field(default_factory=list)
    evidence: list[EvidenceRecord] = field(default_factory=list)

    def add_binary(self, record: BinaryRecord) -> None:
        if any(item.path == record.path for item in self.binaries):
            raise ValueError(f"duplicate binary path: {record.path}")
        self.binaries.append(record)

    def add_dependency(self, record: DependencyRecord) -> None:
        key = (record.parent, record.child, record.kind, record.address)
        if any(
            (item.parent, item.child, item.kind, item.address) == key
            for item in self.dependencies
        ):
            raise ValueError(f"duplicate dependency: {record.parent} -> {record.child}")
        self.dependencies.append(record)

    def add_evidence(self, record: EvidenceRecord) -> None:
        if any(item.evidence_id == record.evidence_id for item in self.evidence):
            raise ValueError(f"duplicate evidence id: {record.evidence_id}")
        self.evidence.append(record)

    def to_dict(self) -> dict[str, object]:
        return {
            "source_root": self.source_root,
            "binaries": [
                asdict(item) for item in sorted(self.binaries, key=lambda item: item.path)
            ],
            "dependencies": [
                asdict(item)
                for item in sorted(
                    self.dependencies,
                    key=lambda item: (item.parent, item.child, item.kind, item.address),
                )
            ],
            "evidence": [
                asdict(item)
                for item in sorted(self.evidence, key=lambda item: item.evidence_id)
            ],
        }

    def to_json(self) -> str:
        return json.dumps(self.to_dict(), indent=2, ensure_ascii=False) + "\n"

    def write(self, path: Path) -> None:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(self.to_json(), encoding="utf-8", newline="\n")


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(1024 * 1024):
            digest.update(chunk)
    return digest.hexdigest()
