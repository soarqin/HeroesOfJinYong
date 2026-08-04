"""CMake target contract helpers for architecture migration tests."""

from __future__ import annotations

import re


_TARGET_PATTERN = re.compile(
    r"\badd_library\(\s*(hojy_[A-Za-z0-9_]+)", re.MULTILINE
)


def declared_targets(cmake_text: str) -> set[str]:
    return set(_TARGET_PATTERN.findall(cmake_text))


def required_targets() -> set[str]:
    return {
        "hojy_content",
        "hojy_world",
        "hojy_event",
        "hojy_battle",
        "hojy_scene",
    }
