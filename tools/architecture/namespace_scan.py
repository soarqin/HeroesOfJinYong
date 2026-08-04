"""Small source scanner used as a migration and final namespace gate."""

from __future__ import annotations

from pathlib import Path
import re


_PATTERNS = (
    (re.compile(r"namespace\s+hojy::(?:data|mem)\b"), "namespace hojy::"),
    (re.compile(r"\bhojy::(?:data|mem)::"), "qualified namespace"),
    (re.compile(r"[\"'](?:data|mem)/"), "legacy include path"),
)
_TEXT_SUFFIXES = {
    ".cc", ".hh", ".h", ".cpp", ".cxx", ".c", ".ipp", ".py", ".cmake", ".md"
}


def scan_text(text: str, filename: str = "<text>") -> list[str]:
    findings: list[str] = []
    for line_number, line in enumerate(text.splitlines(), start=1):
        for pattern, _description in _PATTERNS:
            if pattern.search(line):
                findings.append(f"{filename}:{line_number}: {line.strip()}")
                break
    return findings


def scan_tree(root: Path, allowlist: set[str] | None = None) -> list[str]:
    allowlist = allowlist or set()
    findings: list[str] = []
    for path in sorted(root.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in _TEXT_SUFFIXES:
            continue
        relative = path.relative_to(root).as_posix()
        if relative in allowlist:
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        findings.extend(scan_text(text, relative))
    if findings:
        raise RuntimeError("legacy namespace references found:\n" + "\n".join(findings))
    return findings
