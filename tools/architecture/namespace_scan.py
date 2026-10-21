"""Source scanner used as a migration and final namespace gate."""

from __future__ import annotations

from pathlib import Path
import re

try:
    from .source_scan import mask_non_code, strip_comments
except ImportError:  # pragma: no cover - supports direct tool invocation
    from source_scan import mask_non_code, strip_comments


_NAMESPACE_PATTERNS = (
    re.compile(r"\bnamespace\s+hojy\s*::\s*(?:data|mem)\b"),
    re.compile(r"\bnamespace\s+hojy\s*\{\s*namespace\s+(?:data|mem)\b"),
    re.compile(r"\bhojy\s*::\s*(?:data|mem)\s*::"),
)
_INCLUDE_PATTERN = re.compile(
    r"(?m)^\s*#\s*include\s*[<\"]\s*(?:data|mem)/"
)
_TEXT_SUFFIXES = {
    ".cc", ".hh", ".h", ".cpp", ".cxx", ".c", ".hpp", ".hxx",
    ".ipp", ".inl", ".inc", ".py", ".cmake", ".md", ".rst", ".txt",
    ".toml", ".json", ".yaml", ".yml",
}


def _line_number(text: str, index: int) -> int:
    return text.count("\n", 0, index) + 1


def _include_positions(text: str) -> list[int]:
    """Find actual preprocessor includes, excluding multiline literals."""
    without_comments = strip_comments(text)
    code = mask_non_code(text)
    positions: list[int] = []
    for match in _INCLUDE_PATTERN.finditer(without_comments):
        line_start = without_comments.rfind("\n", 0, match.start()) + 1
        first_code = line_start
        while first_code < len(code) and code[first_code] in " \t\r":
            first_code += 1
        # A literal line is fully masked, while the '#' of a real directive
        # remains code.  This avoids reporting #include text in raw strings.
        if first_code >= len(code) or code[first_code] != "#":
            continue
        positions.append(match.start())
    return positions


def scan_text(text: str, filename: str = "<text>") -> list[str]:
    """Return one finding per source line containing a legacy reference."""
    code = mask_non_code(text)
    positions = [
        match.start()
        for pattern in _NAMESPACE_PATTERNS
        for match in pattern.finditer(code)
    ]
    positions.extend(_include_positions(text))

    findings: list[str] = []
    seen_lines: set[int] = set()
    for index in sorted(positions):
        line_number = _line_number(text, index)
        if line_number in seen_lines:
            continue
        seen_lines.add(line_number)
        lines = text.splitlines()
        line = lines[line_number - 1].strip() if line_number <= len(lines) else ""
        findings.append(f"{filename}:{line_number}: {line}")
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
        except UnicodeDecodeError as error:
            raise RuntimeError(f"unable to decode source file: {relative}") from error
        except OSError as error:
            raise RuntimeError(f"unable to read source file: {relative}") from error
        findings.extend(scan_text(text, relative))
    if findings:
        raise RuntimeError("legacy namespace references found:\n" + "\n".join(findings))
    return findings
