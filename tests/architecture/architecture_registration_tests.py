"""Keep architecture Python gates discoverable and registered with CTest."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]
ARCH_ROOT = ROOT / "tests" / "architecture"


def cmake_text() -> str:
    return (ROOT / "tests" / "CMakeLists.txt").read_text(encoding="utf-8")


def architecture_scripts() -> set[str]:
    return {
        path.relative_to(ARCH_ROOT).as_posix()
        for path in ARCH_ROOT.rglob("*_tests.py")
    }


def registered_architecture_blocks(text: str) -> list[tuple[str, str, str]]:
    blocks: list[tuple[str, str, str]] = []
    command_pattern = re.compile(r"add_test\s*\((?P<body>.*?)\)", re.DOTALL)
    for match in command_pattern.finditer(text):
        body = match.group("body")
        script_match = re.search(r"tests/architecture/([^\s)]+\.py)", body)
        if not script_match:
            continue
        name_match = re.search(r"\bNAME\s+([A-Za-z0-9_]+)", body)
        if not name_match:
            raise AssertionError("architecture add_test is missing NAME")
        blocks.append((name_match.group(1), script_match.group(1), body))
    return blocks


def registered_properties(text: str) -> dict[str, str]:
    result: dict[str, str] = {}
    pattern = re.compile(
        r"set_tests_properties\s*\(\s*([A-Za-z0-9_]+)\s+PROPERTIES(?P<body>.*?)\)",
        re.DOTALL,
    )
    for match in pattern.finditer(text):
        result[match.group(1)] = match.group("body")
    return result


class ArchitectureRegistrationTests(unittest.TestCase):
    def test_every_architecture_script_has_exactly_one_ctest_registration(self) -> None:
        text = cmake_text()
        blocks = registered_architecture_blocks(text)
        by_script: dict[str, list[tuple[str, str, str]]] = {}
        for block in blocks:
            by_script.setdefault(block[1], []).append(block)

        scripts = architecture_scripts()
        self.assertEqual(set(by_script), scripts)
        for script in sorted(scripts):
            self.assertEqual(len(by_script[script]), 1, script)

    def test_architecture_registration_names_are_stable(self) -> None:
        for name, script, body in registered_architecture_blocks(cmake_text()):
            stem = Path(script).stem
            expected = stem if stem.startswith("architecture_") else "architecture_" + stem
            self.assertEqual(name, expected)
            self.assertIn("-m unittest", body)
            self.assertIn("-v", body)

    def test_architecture_tests_run_from_repository_root(self) -> None:
        properties = registered_properties(cmake_text())
        for name, _script, _body in registered_architecture_blocks(cmake_text()):
            self.assertIn(name, properties, name)
            self.assertIn("WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}", properties[name], name)

    def test_python_is_required_before_architecture_tests_are_defined(self) -> None:
        text = cmake_text()
        find_position = text.find("find_package(Python3 COMPONENTS Interpreter REQUIRED)")
        self.assertGreaterEqual(find_position, 0)
        first_architecture = text.find("architecture_", find_position)
        self.assertGreaterEqual(first_architecture, find_position)


if __name__ == "__main__":
    unittest.main()
