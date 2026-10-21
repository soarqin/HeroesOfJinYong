"""Small lexical helpers shared by architecture source scanners."""

from __future__ import annotations

from pathlib import Path
from typing import Iterable


_RAW_PREFIXES = ("u8R\"", "uR\"", "UR\"", "LR\"", "R\"")
_QUOTED_PREFIXES = ("u8\"", "u\"", "U\"", "L\"", "\"")
_CHAR_PREFIXES = ("u8'", "u'", "U'", "L'", "'")


def _raw_literal_end(text: str, start: int) -> int | None:
    prefix = next((value for value in _RAW_PREFIXES if text.startswith(value, start)), None)
    if prefix is None:
        return None

    quote = start + len(prefix) - 1
    opening = text.find("(", quote + 1)
    if opening < 0 or opening - quote - 1 > 16:
        return None
    delimiter = text[quote + 1:opening]
    if any(char.isspace() or char in "()\\" for char in delimiter):
        return None
    closing = ")" + delimiter + '"'
    end = text.find(closing, opening + 1)
    return len(text) if end < 0 else end + len(closing)


def _quoted_literal_end(text: str, start: int, prefixes: tuple[str, ...]) -> int | None:
    prefix = next((value for value in prefixes if text.startswith(value, start)), None)
    if prefix is None:
        return None
    quote = prefix[-1]
    index = start + len(prefix)
    while index < len(text):
        char = text[index]
        if char == "\\":
            index += 2
            continue
        if char == quote:
            return index + 1
        index += 1
    return len(text)


def strip_comments(text: str) -> str:
    """Remove C/C++ comments while preserving literals and line positions."""
    result = list(text)
    index = 0
    while index < len(text):
        raw_end = _raw_literal_end(text, index)
        if raw_end is not None:
            index = raw_end
            continue
        quoted_end = _quoted_literal_end(text, index, _QUOTED_PREFIXES)
        if quoted_end is not None:
            index = quoted_end
            continue
        char_end = _quoted_literal_end(text, index, _CHAR_PREFIXES)
        if char_end is not None:
            index = char_end
            continue
        if text.startswith("//", index):
            result[index] = result[index + 1] = " "
            index += 2
            while index < len(text) and text[index] not in "\r\n":
                result[index] = " "
                index += 1
            continue
        if text.startswith("/*", index):
            result[index] = result[index + 1] = " "
            index += 2
            while index < len(text):
                if text.startswith("*/", index):
                    result[index] = result[index + 1] = " "
                    index += 2
                    break
                if text[index] not in "\r\n":
                    result[index] = " "
                index += 1
            continue
        index += 1
    return "".join(result)


def mask_non_code(text: str) -> str:
    """Replace comments and literals with spaces, preserving every index."""
    result = list(text)
    index = 0

    def blank(start: int, end: int) -> None:
        for position in range(start, min(end, len(result))):
            if result[position] not in "\r\n":
                result[position] = " "

    while index < len(text):
        raw_end = _raw_literal_end(text, index)
        if raw_end is not None:
            blank(index, raw_end)
            index = raw_end
            continue
        quoted_end = _quoted_literal_end(text, index, _QUOTED_PREFIXES)
        if quoted_end is not None:
            blank(index, quoted_end)
            index = quoted_end
            continue
        char_end = _quoted_literal_end(text, index, _CHAR_PREFIXES)
        if char_end is not None:
            blank(index, char_end)
            index = char_end
            continue
        if text.startswith("//", index):
            end = index + 2
            while end < len(text) and text[end] not in "\r\n":
                end += 1
            blank(index, end)
            index = end
            continue
        if text.startswith("/*", index):
            end = text.find("*/", index + 2)
            end = len(text) if end < 0 else end + 2
            blank(index, end)
            index = end
            continue
        index += 1
    return "".join(result)


def matching_brace(text: str, opening_brace: int) -> int:
    """Return the closing brace for a code brace, ignoring literals/comments."""
    if opening_brace < 0 or opening_brace >= len(text) or text[opening_brace] != "{":
        raise AssertionError("opening_brace must point to '{'")

    depth = 0
    index = opening_brace
    while index < len(text):
        raw_end = _raw_literal_end(text, index)
        if raw_end is not None:
            index = raw_end
            continue
        quoted_end = _quoted_literal_end(text, index, _QUOTED_PREFIXES)
        if quoted_end is not None:
            index = quoted_end
            continue
        char_end = _quoted_literal_end(text, index, _CHAR_PREFIXES)
        if char_end is not None:
            index = char_end
            continue
        if text.startswith("//", index):
            newline = text.find("\n", index + 2)
            index = len(text) if newline < 0 else newline + 1
            continue
        if text.startswith("/*", index):
            end = text.find("*/", index + 2)
            index = len(text) if end < 0 else end + 2
            continue
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return index
        index += 1
    raise AssertionError("unterminated brace block")


def function_body(text: str, signature: str) -> str:
    """Extract a function body using code-aware signature and brace matching."""
    masked = mask_non_code(text)
    search_from = 0
    found_signature = False
    while True:
        start = masked.find(signature, search_from)
        if start < 0:
            break
        found_signature = True
        search_from = start + 1

        # A short signature such as ``void Foo::run`` must not match
        # ``void Foo::runner``.  Keep this boundary check deliberately small:
        # callers may pass a prefix that stops before qualifiers or a body.
        before = masked[start - 1] if start else " "
        after_index = start + len(signature)
        after = masked[after_index] if after_index < len(masked) else " "
        if (before.isalnum() or before == "_") or (
            signature and (signature[-1].isalnum() or signature[-1] == "_")
            and (after.isalnum() or after == "_")
        ):
            continue

        opening_brace = None
        terminator = None
        index = after_index
        while index < len(masked):
            if masked[index] in "{;":
                terminator = masked[index]
                if terminator == "{":
                    opening_brace = index
                break
            index += 1
        if terminator != "{" or opening_brace is None:
            # This occurrence was a declaration/prototype.  Continue looking
            # for the out-of-line definition later in the file.
            continue
        closing_brace = matching_brace(text, opening_brace)
        return text[opening_brace + 1:closing_brace]
    if not found_signature:
        raise AssertionError(f"function signature not found: {signature}")
    raise AssertionError(f"function body not found: {signature}")


def read_function_body(path: Path | str, signature: str) -> str:
    return function_body(Path(path).read_text(encoding="utf-8"), signature)


def find_function_body(
        paths: Iterable[Path] | Path | str, signature: str) -> tuple[Path, str]:
    if isinstance(paths, (Path, str)):
        root = Path(paths)
        candidates = [root] if root.is_file() else sorted(root.rglob("*"))
    else:
        candidates = paths
    for candidate in candidates:
        path = Path(candidate)
        if not path.is_file():
            continue
        text = path.read_text(encoding="utf-8")
        masked = mask_non_code(text)
        if masked.find(signature) >= 0:
            try:
                return path, function_body(text, signature)
            except AssertionError:
                # A declaration/header match is not enough; keep searching
                # for a file containing the actual definition.
                continue
    raise AssertionError(f"function signature not found in paths: {signature}")
