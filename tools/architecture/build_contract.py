"""CMake target contracts used by the architecture migration gates."""

from __future__ import annotations

from pathlib import Path
import re
from typing import Iterable


MAIN_TARGET = "HeroesOfJinYongMain"
_COMMAND_PATTERN = re.compile(
    r"(?im)(?:^|[;\r\n])[ \t]*([A-Za-z_][A-Za-z0-9_]*)\s*\("
)
_SCOPE_KEYWORDS = {"PUBLIC", "PRIVATE", "INTERFACE", "LINK_PUBLIC", "LINK_PRIVATE"}
_LIBRARY_KINDS = {
    "STATIC", "SHARED", "MODULE", "OBJECT", "INTERFACE", "UNKNOWN", "IMPORTED",
}
_EXECUTABLE_KINDS = {"WIN32", "MACOSX_BUNDLE", "EXCLUDE_FROM_ALL", "IMPORTED"}
_SOURCE_SUFFIXES = {".cc", ".cpp", ".cxx", ".c", ".hh", ".h", ".hpp", ".hxx"}


def _bracket_end(text: str, start: int) -> int | None:
    """Return the end of a CMake bracket argument beginning at ``start``."""
    match = re.match(r"\[(=*)\[", text[start:])
    if match is None:
        return None
    opening = match.group(0)
    closing = "]" + match.group(1) + "]"
    end = text.find(closing, start + len(opening))
    return len(text) if end < 0 else end + len(closing)


def _strip_cmake_comments(text: str) -> str:
    result = list(text)
    index = 0
    while index < len(text):
        bracket_end = _bracket_end(text, index)
        if bracket_end is not None:
            # Bracket arguments are opaque to CMake's comment lexer.  In
            # particular, a '#' inside one must not start a line comment.
            index = bracket_end
            continue
        if text[index] == '"':
            index += 1
            while index < len(text):
                if text[index] == "\\":
                    index += 2
                    continue
                if text[index] == '"':
                    index += 1
                    break
                index += 1
            continue
        if text[index] == "#":
            comment_end = _bracket_end(text, index + 1)
            if comment_end is not None:
                for position in range(index, comment_end):
                    if result[position] not in "\r\n":
                        result[position] = " "
                index = comment_end
                continue
            index += 1
            while index < len(text) and text[index] not in "\r\n":
                result[index] = " "
                index += 1
            continue
        index += 1
    return "".join(result)


def _matching_parenthesis(text: str, opening: int) -> int:
    depth = 0
    index = opening
    while index < len(text):
        bracket_end = _bracket_end(text, index)
        if bracket_end is not None:
            index = bracket_end
            continue
        if text[index] == '"':
            index += 1
            while index < len(text):
                if text[index] == "\\":
                    index += 2
                    continue
                if text[index] == '"':
                    index += 1
                    break
                index += 1
            continue
        if text[index] == "(":
            depth += 1
        elif text[index] == ")":
            depth -= 1
            if depth == 0:
                return index
        index += 1
    raise ValueError("unterminated CMake command")


def _mask_cmake_literals(text: str) -> str:
    """Mask quoted and bracket arguments while preserving source offsets."""
    result = list(text)
    index = 0

    def blank(start: int, end: int) -> None:
        for position in range(start, min(end, len(result))):
            if result[position] not in "\r\n":
                result[position] = " "

    while index < len(text):
        bracket_end = _bracket_end(text, index)
        if bracket_end is not None:
            blank(index, bracket_end)
            index = bracket_end
            continue
        if text[index] == '"':
            start = index
            index += 1
            while index < len(text):
                if text[index] == "\\":
                    index += 2
                    continue
                if text[index] == '"':
                    index += 1
                    break
                index += 1
            blank(start, index)
            continue
        index += 1
    return "".join(result)


def _commands(cmake_text: str) -> Iterable[tuple[str, str]]:
    clean = _strip_cmake_comments(cmake_text)
    searchable = _mask_cmake_literals(clean)
    for match in _COMMAND_PATTERN.finditer(searchable):
        opening = clean.find("(", match.start(), match.end())
        yield match.group(1).lower(), clean[opening + 1:_matching_parenthesis(clean, opening)]


def _tokens(body: str) -> list[str]:
    """Tokenize a CMake command and expand literal semicolon list items."""
    raw_tokens: list[str] = []
    index = 0
    while index < len(body):
        while index < len(body) and body[index].isspace():
            index += 1
        if index >= len(body):
            break
        if body[index] == '"':
            start = index
            index += 1
            while index < len(body):
                if body[index] == "\\":
                    index += 2
                    continue
                if body[index] == '"':
                    index += 1
                    break
                index += 1
            token = body[start + 1:index - 1]
            token = token.replace('\\"', '"').replace('\\\\', '\\')
            raw_tokens.append(token)
            continue
        bracket_end = _bracket_end(body, index)
        if bracket_end is not None:
            raw_tokens.append(body[index:bracket_end])
            index = bracket_end
            continue
        start = index
        while index < len(body) and not body[index].isspace():
            index += 1
        raw_tokens.append(body[start:index])

    result: list[str] = []
    for token in raw_tokens:
        current: list[str] = []
        index = 0
        while index < len(token):
            if token[index] == "\\" and index + 1 < len(token) and token[index + 1] == ";":
                current.append(";")
                index += 2
                continue
            if token[index] == ";":
                if current:
                    result.append("".join(current))
                    current = []
                index += 1
                continue
            current.append(token[index])
            index += 1
        if current:
            result.append("".join(current))
    return result


def _project_name(cmake_text: str) -> str | None:
    for command, body in _commands(cmake_text):
        if command == "project":
            tokens = _tokens(body)
            if tokens:
                return tokens[0]
    return None


def _resolve_target(token: str, project_name: str | None) -> str | None:
    if token == "${PROJECT_NAME}":
        return project_name
    if token.startswith("${") and token.endswith("}"):
        return None
    return token


def _normalise_path(token: str) -> str:
    token = token.replace("\\", "/")
    token = token.replace("${CMAKE_CURRENT_SOURCE_DIR}/", "")
    token = token.replace("${PROJECT_SOURCE_DIR}/", "")
    while token.startswith("./"):
        token = token[2:]
    return token


def _file_globs(cmake_text: str) -> tuple[dict[str, list[str]], dict[str, set[str]]]:
    variables: dict[str, list[str]] = {}
    removed: dict[str, set[str]] = {}
    for command, body in _commands(cmake_text):
        tokens = _tokens(body)
        if command != "file" or len(tokens) < 3 or tokens[0].upper() not in {"GLOB", "GLOB_RECURSE"}:
            continue
        variable = tokens[1]
        variables[variable] = [
            _normalise_path(token)
            for token in tokens[2:]
            if token.upper() != "CONFIGURE_DEPENDS"
        ]

    for command, body in _commands(cmake_text):
        tokens = _tokens(body)
        if command != "list" or len(tokens) < 3 or tokens[0].upper() != "REMOVE_ITEM":
            continue
        variable = tokens[1]
        if variable not in variables:
            continue
        items = {_normalise_path(token) for token in tokens[2:]}
        removed.setdefault(variable, set()).update(items)
    return variables, removed


def _expand_source_token(
        token: str,
        variables: dict[str, list[str]],
        source_root: Path | None,
        removed: dict[str, set[str]],
        seen: set[str] | None = None) -> set[str]:
    seen = set() if seen is None else seen
    variable_match = re.fullmatch(r"\$\{([A-Za-z_][A-Za-z0-9_]*)\}", token)
    if variable_match:
        variable = variable_match.group(1)
        if variable in seen:
            return set()
        expanded = {
            path
            for item in variables.get(variable, [])
            for path in _expand_source_token(
                item, variables, source_root, removed, seen | {variable}
            )
        }
        return expanded - removed.get(variable, set())
    if token.startswith("$<"):
        return set()

    path = _normalise_path(token)
    if source_root is None:
        return {path}
    path_object = Path(path)
    if path_object.is_absolute():
        try:
            path = path_object.relative_to(source_root).as_posix()
        except ValueError:
            return set()
    if any(char in path for char in "*?["):
        return {
            candidate.relative_to(source_root).as_posix()
            for candidate in source_root.glob(path)
            if candidate.is_file()
        }
    candidate = source_root / path
    return {path} if candidate.is_file() else set()


def _target_source_tokens(cmake_text: str) -> dict[str, list[str]]:
    project_name = _project_name(cmake_text)
    targets: dict[str, list[str]] = {}
    for command, body in _commands(cmake_text):
        if command not in {"add_library", "add_executable"}:
            continue
        tokens = _tokens(body)
        if not tokens:
            continue
        target = _resolve_target(tokens[0], project_name)
        if target is None:
            continue
        index = 1
        kinds = _LIBRARY_KINDS if command == "add_library" else _EXECUTABLE_KINDS
        while index < len(tokens) and tokens[index].upper() in kinds:
            index += 1
        if command == "add_library" and index < len(tokens) and tokens[index].upper() == "ALIAS":
            # An ALIAS/IMPORTED target does not own source files.  Keep the
            # declaration visible while excluding its metadata tokens from
            # ownership accounting.
            targets.setdefault(target, [])
            continue
        if index < len(tokens) and tokens[index].upper() == "GLOBAL":
            targets.setdefault(target, [])
            continue
        targets.setdefault(target, []).extend(tokens[index:])
    return targets


def declared_targets(cmake_text: str) -> set[str]:
    """Return uncommented library and executable target names."""
    return set(_target_source_tokens(cmake_text))


def required_targets() -> set[str]:
    return {
        "hojy_content",
        "hojy_world",
        "hojy_event",
        "hojy_battle",
        "hojy_scene_logic",
        "hojy_scene",
        "hojy_platform",
        "hojy_app",
        MAIN_TARGET,
    }


def target_links(cmake_text: str) -> dict[str, set[str]]:
    project_name = _project_name(cmake_text)
    links: dict[str, set[str]] = {}
    for command, body in _commands(cmake_text):
        if command != "target_link_libraries":
            continue
        tokens = _tokens(body)
        if not tokens:
            continue
        target = _resolve_target(tokens[0], project_name)
        if target is None:
            continue
        links.setdefault(target, set()).update(
            token for token in tokens[1:]
            if token.upper() not in _SCOPE_KEYWORDS and not token.startswith("$<")
        )
    return links


def required_dependencies() -> dict[str, set[str]]:
    return {
        "hojy_platform": {"SDL2::SDL2"},
        "hojy_world": {"hojy_content"},
        "hojy_event": {"hojy_world"},
        "hojy_scene": {
            "hojy_scene_logic", "hojy_event", "hojy_battle", "hojy_world",
            "hojy_content", "hojy_platform",
        },
        "hojy_app": {"hojy_scene"},
        MAIN_TARGET: {"hojy_app", "hojy_scene", "hojy_event", "hojy_world", "hojy_content"},
    }


def allowed_project_dependencies() -> dict[str, set[str]]:
    """Return the permitted links to other first-party ``hojy_*`` targets."""
    return {
        "hojy_platform": set(),
        "hojy_content": set(),
        "hojy_world": {"hojy_content"},
        "hojy_event": {"hojy_world"},
        "hojy_battle": set(),
        "hojy_scene_logic": set(),
        "hojy_scene": {
            "hojy_scene_logic", "hojy_event", "hojy_battle", "hojy_world",
            "hojy_content", "hojy_platform",
        },
        "hojy_app": {"hojy_scene"},
        MAIN_TARGET: {"hojy_app", "hojy_scene", "hojy_event", "hojy_world", "hojy_content"},
    }


def source_ownership(cmake_text: str, source_root: Path | None = None) -> dict[str, set[str]]:
    variables, removed = _file_globs(cmake_text)
    ownership: dict[str, set[str]] = {}
    for target, tokens in _target_source_tokens(cmake_text).items():
        owned: set[str] = set()
        for token in tokens:
            owned.update(_expand_source_token(token, variables, source_root, removed))
        ownership[target] = owned
    return ownership


_EXPECTED_PREFIXES = (
    ("scene/logic/", "hojy_scene_logic"),
    ("scene/", "hojy_scene"),
    ("app/text_input.", "hojy_platform"),
    ("app/", "hojy_app"),
    ("content/", "hojy_content"),
    ("world/", "hojy_world"),
    ("event/", "hojy_event"),
    ("battle/", "hojy_battle"),
    ("core/", MAIN_TARGET),
    ("audio/", MAIN_TARGET),
    ("util/", MAIN_TARGET),
    ("main.cc", MAIN_TARGET),
)


def _expected_owner(relative: str) -> str | None:
    for prefix, target in _EXPECTED_PREFIXES:
        if relative == prefix or relative.startswith(prefix):
            return target
    return None


def validate_contract(cmake_text: str, source_root: Path | None = None) -> list[str]:
    findings: list[str] = []
    targets = declared_targets(cmake_text)
    for target in sorted(required_targets() - targets):
        findings.append(f"missing target: {target}")

    links = target_links(cmake_text)
    for target, dependencies in required_dependencies().items():
        missing = dependencies - links.get(target, set())
        for dependency in sorted(missing):
            findings.append(f"missing dependency: {target} -> {dependency}")
    for target, allowed in allowed_project_dependencies().items():
        project_links = {
            dependency for dependency in links.get(target, set())
            if dependency.startswith("hojy_")
        }
        for dependency in sorted(project_links - allowed):
            findings.append(f"forbidden dependency: {target} -> {dependency}")

    if source_root is None:
        return findings

    ownership = source_ownership(cmake_text, source_root)
    source_targets: dict[str, set[str]] = {}
    runtime_targets = required_targets()
    for target, paths in ownership.items():
        if target not in runtime_targets:
            continue
        for path in paths:
            source_targets.setdefault(path, set()).add(target)
    for path, owners in sorted(source_targets.items()):
        if len(owners) > 1:
            findings.append(f"source owned by multiple targets: {path}: {sorted(owners)}")

    for path in sorted(source_root.rglob("*")):
        if not path.is_file() or path.suffix.lower() not in _SOURCE_SUFFIXES:
            continue
        relative = path.relative_to(source_root).as_posix()
        expected = _expected_owner(relative)
        if expected is None:
            continue
        owners = source_targets.get(relative, set())
        if owners != {expected}:
            findings.append(
                f"source ownership mismatch: {relative}: expected {expected}, got {sorted(owners)}"
            )
    return findings
