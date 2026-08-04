import unittest
from pathlib import Path

from build_contract import declared_targets, required_targets


class BuildContractTests(unittest.TestCase):
    def test_architecture_libraries_are_declared(self):
        targets = declared_targets(Path('src/CMakeLists.txt').read_text(encoding='utf-8'))
        self.assertTrue(required_targets().issubset(targets))


if __name__ == '__main__':
    unittest.main()
