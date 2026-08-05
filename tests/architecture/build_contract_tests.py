import unittest
from pathlib import Path
import tempfile

from tools.architecture.build_contract import (
    declared_targets,
    required_dependencies,
    required_targets,
    source_ownership,
    target_links,
    validate_contract,
)
from tools.architecture.source_scan import function_body


ROOT = Path(__file__).resolve().parents[2]


class BuildContractTests(unittest.TestCase):
    def test_architecture_libraries_are_declared(self):
        targets = declared_targets(
            (ROOT / 'src' / 'CMakeLists.txt').read_text(encoding='utf-8')
        )
        self.assertTrue(required_targets().issubset(targets))

    def test_runtime_target_contract_is_complete(self):
        cmake = (ROOT / 'src' / 'CMakeLists.txt').read_text(encoding='utf-8')
        self.assertEqual(validate_contract(cmake, ROOT / 'src'), [])

    def test_target_links_resolve_project_name_and_ignore_comments(self):
        cmake = '''
        project(Demo CXX)
        # add_library(hojy_fake STATIC fake.cc)
        add_library(hojy_scene_logic STATIC logic.cc)
        add_executable(${PROJECT_NAME} main.cc)
        target_link_libraries(${PROJECT_NAME} PRIVATE hojy_scene hojy_app)
        '''
        self.assertEqual(
            declared_targets(cmake), {'Demo', 'hojy_scene_logic'}
        )
        self.assertEqual(
            target_links(cmake)['Demo'], {'hojy_scene', 'hojy_app'}
        )

    def test_cmake_parser_ignores_bracket_comments_and_splits_list_values(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'scene').mkdir()
            for name in ('first.cc', 'second.cc'):
                (root / 'scene' / name).write_text('', encoding='utf-8')
            cmake = '''
            #[[
            add_library(hojy_fake STATIC scene/fake.cc)
            ]]
            set(DOC [=[
            add_library(hojy_fake_in_literal STATIC scene/fake.cc)
            ]=])
            add_library(hojy_scene STATIC
                "scene/first.cc;scene/second.cc")
            '''
            self.assertEqual(declared_targets(cmake), {'hojy_scene'})
            self.assertEqual(
                source_ownership(cmake, root)['hojy_scene'],
                {'scene/first.cc', 'scene/second.cc'},
            )

    def test_required_dependencies_include_all_phase_layers(self):
        dependencies = required_dependencies()
        self.assertIn('hojy_scene_logic', dependencies['hojy_scene'])
        self.assertIn('hojy_platform', dependencies['hojy_scene'])
        self.assertEqual(dependencies['hojy_app'], {'hojy_scene'})
        self.assertIn('hojy_app', dependencies['HeroesOfJinYongMain'])

    def test_contract_rejects_a_reverse_platform_dependency(self):
        cmake = (ROOT / 'src' / 'CMakeLists.txt').read_text(encoding='utf-8')
        cmake = cmake.replace(
            'target_link_libraries(hojy_platform PUBLIC SDL2::SDL2)',
            'target_link_libraries(hojy_platform PUBLIC SDL2::SDL2 hojy_scene)',
        )
        findings = validate_contract(cmake, ROOT / 'src')
        self.assertIn(
            'forbidden dependency: hojy_platform -> hojy_scene', findings
        )

    def test_source_ownership_expands_globs_and_removes_reassigned_files(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'scene' / 'logic').mkdir(parents=True)
            (root / 'app').mkdir()
            for relative in ('scene/map.cc', 'scene/logic/input.cc',
                             'app/main.cc', 'app/text_input.cc'):
                path = root / relative
                path.write_text('', encoding='utf-8')
            cmake = '''
            file(GLOB SCENE_FILES scene/*.cc)
            file(GLOB SCENE_LOGIC_FILES scene/logic/*.cc)
            file(GLOB APP_FILES app/*.cc)
            list(REMOVE_ITEM APP_FILES
                "${CMAKE_CURRENT_SOURCE_DIR}/app/text_input.cc")
            add_library(hojy_scene STATIC ${SCENE_FILES})
            add_library(hojy_scene_logic STATIC ${SCENE_LOGIC_FILES})
            add_library(hojy_platform STATIC app/text_input.cc)
            add_library(hojy_app STATIC ${APP_FILES})
            '''
            ownership = source_ownership(cmake, root)
            self.assertEqual(ownership['hojy_scene'], {'scene/map.cc'})
            self.assertEqual(ownership['hojy_scene_logic'],
                             {'scene/logic/input.cc'})
            self.assertEqual(ownership['hojy_platform'], {'app/text_input.cc'})
            self.assertEqual(ownership['hojy_app'], {'app/main.cc'})

    def test_function_body_parser_ignores_comments_strings_and_characters(self):
        body = function_body(
            '''
            void Example::run() {
                const char *text = "a } // not a comment";
                const char brace = '}';
                /* { this is a comment } */
                if (text) { return; }
            }
            ''',
            'void Example::run',
        )
        self.assertIn('if (text) { return; }', body)
        self.assertNotIn('void Example::next', body)

    def test_function_body_parser_skips_declarations_and_raw_strings(self):
        body = function_body(
            '''
            void Example::run();
            void Example::other() { throw 1; }
            void Example::run() {
                const char *text = R"marker({ } // still text)marker";
                return;
            }
            ''',
            'void Example::run',
        )
        self.assertIn('R"marker({ } // still text)marker"', body)
        self.assertIn('return;', body)
        self.assertNotIn('throw 1', body)


if __name__ == '__main__':
    unittest.main()
