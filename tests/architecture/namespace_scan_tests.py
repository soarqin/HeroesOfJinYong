import tempfile
import unittest
from pathlib import Path

from tools.architecture.namespace_scan import scan_text, scan_tree


ROOT = Path(__file__).resolve().parents[2]
LEGACY_STATE = "m" + "em"
LEGACY_CONTENT = "da" + "ta"


class NamespaceScanTests(unittest.TestCase):
    def test_scan_text_reports_legacy_namespace_and_qualified_name(self):
        state_namespace = f"namespace hojy::{LEGACY_STATE}"
        content_name = f"hojy::{LEGACY_CONTENT}::GrpData"
        findings = scan_text(
            f"{state_namespace} {{ int value; }}\n{content_name} value;\n",
            'sample.cc',
        )
        self.assertEqual(
            findings,
            [
                f'sample.cc:1: {state_namespace} {{ int value; }}',
                f'sample.cc:2: {content_name} value;',
            ],
        )

    def test_scan_tree_supports_explicit_allowlist(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'legacy.cc').write_text(
                f'namespace hojy::{LEGACY_STATE} {{}}\n', encoding='utf-8'
            )
            (root / 'new.cc').write_text(
                'namespace hojy::world {}\n', encoding='utf-8'
            )
            self.assertEqual(
                scan_tree(root, allowlist={'legacy.cc'}),
                [],
            )

    def test_scan_tree_reports_non_allowlisted_files(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'legacy.cc').write_text(
                f'namespace hojy::{LEGACY_STATE} {{}}\n', encoding='utf-8'
            )
            with self.assertRaisesRegex(RuntimeError, 'legacy.cc:1'):
                scan_tree(root)

    def test_scan_text_ignores_comments_and_non_include_string_literals(self):
        text = '''
        // namespace hojy::mem { }
        const char *name = "hojy::data::GrpData";
        const char *path = "data/font/chinese.otf";
        '''
        self.assertEqual(scan_text(text, 'sample.cc'), [])

    def test_scan_text_ignores_include_text_inside_raw_string(self):
        text = '''
        const char *source = R"source(
        #include <data/grpdata.hh>
        namespace hojy::mem {}
        )source";
        '''
        self.assertEqual(scan_text(text, 'sample.cc'), [])

    def test_scan_text_detects_split_legacy_namespace_and_include_directive(self):
        include = '#include <' + LEGACY_CONTENT + '/grpdata.hh>'
        text = '''
        namespace hojy {
        namespace mem { int value; }
        }
        ''' + include + '\n'
        findings = scan_text(text, 'sample.cc')
        self.assertEqual(
            [finding.split(':', 2)[1] for finding in findings], ['2', '5']
        )

    def test_scan_tree_fails_closed_on_undecodable_source(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'broken.cc').write_bytes(b'\xffnamespace hojy::mem {}')
            with self.assertRaisesRegex(RuntimeError, 'broken.cc'):
                scan_tree(root)

    def test_repository_source_tree_has_no_legacy_namespace_or_include(self):
        self.assertEqual(scan_tree(ROOT / 'src'), [])
        self.assertEqual(scan_tree(ROOT / 'tests'), [])

        for path in (
            ROOT / 'CMakeLists.txt',
            ROOT / 'src' / 'CMakeLists.txt',
            ROOT / 'tests' / 'CMakeLists.txt',
        ):
            self.assertEqual(
                scan_text(path.read_text(encoding='utf-8'), str(path.relative_to(ROOT))),
                [],
            )


if __name__ == '__main__':
    unittest.main()
