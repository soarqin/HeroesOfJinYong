# Battle Reverse-Engineering Contract Baseline Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 建立可重复运行的原版战斗逆向工具、证据清单、确定性随机源与自动化测试入口，为后续战斗核心和 AI 重构提供行为契约。

**Architecture:** C++ 侧新增独立 `hojy_battle` 静态库与 CTest 测试目标，先放入不依赖 SDL 的随机源接口。Python 侧把文件哈希、清单模型与 idalib 访问分离：纯模块使用 `unittest` 测试，idalib 命令行负责读取原版目录并生成结构化 JSON。逆向文档只保存地址、交叉引用、函数摘要和证据状态，不保存原版二进制或大段反编译文本。

**Tech Stack:** C++17、CMake、CTest、Python 3.12、`unittest`、IDA Pro 9.4、idalib、IDAPython API。

---

## File Structure

- `src/battle/random.hh`：战斗随机接口、生产适配器、固定序列随机源和调用记录。
- `src/battle/random.cc`：随机范围校验与固定序列消费。
- `src/battle/game_random.hh`：现有游戏随机源适配器声明。
- `src/battle/game_random.cc`：`util::gRandom` 适配实现。
- `tests/test_support.hh`：无第三方依赖的最小 C++ 测试断言。
- `tests/battle/random_tests.cc`：随机边界、消费顺序和调用记录测试。
- `tests/CMakeLists.txt`：C++ 与 Python 测试注册。
- `tools/reverse/manifest.py`：哈希、证据模型、JSON 序列化和输入校验。
- `tools/reverse/ida_backend.py`：idalib 数据库生命周期、字符串、交叉引用、函数和反编译摘要读取。
- `tools/reverse/extract_battle_evidence.py`：入口解析、依赖跟踪、战斗锚点提取和 JSON 写出。
- `tests/reverse/test_manifest.py`：纯 Python 模块测试。
- `tests/reverse/test_extract_cli.py`：不打开 IDA 数据库的 CLI 参数与哈希校验测试。
- `docs/reverse/battle-evidence.md`：二进制关系、初始函数锚点和证据编号。
- `docs/reverse/battle-behavior-matrix.md`：后续逆向工作的行为清单与状态。
- `docs/reverse/manifests/legend-z-baseline.json`：当前原版文件哈希和可重复提取的结构化结果。

### Task 1: Add CTest and the battle-domain test target

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `src/CMakeLists.txt`
- Create: `tests/CMakeLists.txt`
- Create: `tests/test_support.hh`
- Create: `tests/battle/random_tests.cc`

- [ ] **Step 1: Write the failing random-source test**

Create `tests/test_support.hh` with a small exception-based assertion helper:

```cpp
#pragma once

#include <sstream>
#include <stdexcept>
#include <string>

namespace hojy::test {

template<typename A, typename B>
void checkEqual(const A &actual, const B &expected, const char *actualExpr,
                const char *expectedExpr, const char *file, int line) {
    if (actual == expected) { return; }
    std::ostringstream stream;
    stream << file << ':' << line << ": expected " << expectedExpr
           << ", got " << actualExpr;
    throw std::runtime_error(stream.str());
}

}

#define HOJY_CHECK_EQ(actual, expected) \
    ::hojy::test::checkEqual((actual), (expected), #actual, #expected, __FILE__, __LINE__)
```

Create `tests/battle/random_tests.cc` with the wished-for API:

```cpp
#include "battle/random.hh"
#include "test_support.hh"

#include <iostream>
#include <vector>

int main() {
    try {
        hojy::battle::SequenceRandom random({7, 2, 9});
        HOJY_CHECK_EQ(random.next(10), 7);
        HOJY_CHECK_EQ(random.next(1, 3), 3);
        HOJY_CHECK_EQ(random.next(4), 1);
        HOJY_CHECK_EQ(random.callCount(), 3U);
        HOJY_CHECK_EQ(random.calls()[1].minimum, 1);
        HOJY_CHECK_EQ(random.calls()[1].maximum, 3);
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
```

- [ ] **Step 2: Register the test and verify the missing API fails**

Add `include(CTest)` near the root `project()` declaration and add:

```cmake
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
```

Create `tests/CMakeLists.txt`:

```cmake
add_executable(battle_random_tests battle/random_tests.cc)
target_include_directories(battle_random_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(battle_random_tests PRIVATE hojy_battle)
add_test(NAME battle_random_tests COMMAND battle_random_tests)
```

Run:

```powershell
cmake -S . -B cmake-build-battle -G Ninja `
  -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/c++.exe `
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
```

Expected: CMake fails because target `hojy_battle` does not exist, proving the test requests a new isolated battle component.

- [ ] **Step 3: Add the minimal battle library target**

Modify `src/CMakeLists.txt`。先添加接口库，使 CMake 可以生成测试目标，同时仍由缺失头文件形成预期失败：

```cmake
add_library(hojy_battle INTERFACE)
target_include_directories(hojy_battle INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
```

Add `hojy_battle` to the application link libraries so later migrations use one implementation:

```cmake
target_link_libraries(${PROJECT_NAME} hojy_battle ADLMIDI SDL2_gfx fmt::fmt)
```

- [ ] **Step 4: Reconfigure and verify the test now fails at compilation**

Run:

```powershell
cmake -S . -B cmake-build-battle -G Ninja `
  -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/c++.exe `
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build cmake-build-battle --target battle_random_tests -j 4
```

Expected: compilation fails with `battle/random.hh: No such file or directory`.

- [ ] **Step 5: Keep the expected failure for Task 2**

Do not commit a branch state that cannot build. Continue directly to Task 2 with the compilation failure recorded in the execution notes.

### Task 2: Implement deterministic battle randomness

**Files:**
- Create: `src/battle/random.hh`
- Create: `src/battle/random.cc`
- Create: `src/battle/game_random.hh`
- Create: `src/battle/game_random.cc`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/battle/random_tests.cc`

- [ ] **Step 1: Define the random-source contract**

Create `src/battle/random.hh`:

```cpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace hojy::battle {

struct RandomCall {
    int minimum;
    int maximum;
    int rawValue;
    int result;
};

class RandomSource {
public:
    virtual ~RandomSource() = default;
    virtual int next(int upperExclusive) = 0;
    virtual int next(int minimum, int maximum) = 0;
};

class SequenceRandom final: public RandomSource {
public:
    explicit SequenceRandom(std::initializer_list<int> values);
    explicit SequenceRandom(std::vector<int> values);

    int next(int upperExclusive) override;
    int next(int minimum, int maximum) override;

    [[nodiscard]] std::size_t callCount() const noexcept;
    [[nodiscard]] const std::vector<RandomCall> &calls() const noexcept;

private:
    int consume(int minimum, int maximum);

    std::vector<int> values_;
    std::vector<RandomCall> calls_;
    std::size_t index_ = 0;
};

}
```

- [ ] **Step 2: Extend the failing test for invalid ranges and exhaustion**

Add cases that require `std::invalid_argument` for `next(0)` and `next(4, 3)`, and `std::out_of_range` after the fixed sequence is exhausted. Run:

```powershell
cmake --build cmake-build-battle --target battle_random_tests -j 4
```

Expected: link failure because the declared methods have no implementation.

- [ ] **Step 3: Implement the minimum behavior**

Create `src/battle/random.cc`:

```cpp
#include "random.hh"

#include <stdexcept>
#include <utility>

namespace hojy::battle {

SequenceRandom::SequenceRandom(std::initializer_list<int> values): values_(values) {}
SequenceRandom::SequenceRandom(std::vector<int> values): values_(std::move(values)) {}

int SequenceRandom::next(int upperExclusive) {
    if (upperExclusive <= 0) { throw std::invalid_argument("upperExclusive must be positive"); }
    return consume(0, upperExclusive - 1);
}

int SequenceRandom::next(int minimum, int maximum) {
    if (minimum > maximum) { throw std::invalid_argument("minimum must not exceed maximum"); }
    return consume(minimum, maximum);
}

int SequenceRandom::consume(int minimum, int maximum) {
    if (index_ >= values_.size()) { throw std::out_of_range("battle random sequence exhausted"); }
    const int raw = values_[index_++];
    const int width = maximum - minimum + 1;
    const int normalized = ((raw % width) + width) % width;
    const int result = minimum + normalized;
    calls_.push_back(RandomCall{minimum, maximum, raw, result});
    return result;
}

std::size_t SequenceRandom::callCount() const noexcept { return calls_.size(); }
const std::vector<RandomCall> &SequenceRandom::calls() const noexcept { return calls_; }

}
```

Create `src/battle/game_random.hh`:

```cpp
#pragma once

#include "random.hh"

namespace hojy::battle {

class GameRandom final: public RandomSource {
public:
    int next(int upperExclusive) override;
    int next(int minimum, int maximum) override;
};

}
```

Create `src/battle/game_random.cc`:

```cpp
#include "game_random.hh"

#include "util/random.hh"

#include <stdexcept>

namespace hojy::battle {

int GameRandom::next(int upperExclusive) {
    if (upperExclusive <= 0) { throw std::invalid_argument("upperExclusive must be positive"); }
    return static_cast<int>(util::gRandom(static_cast<util::Random::IntType>(upperExclusive)));
}

int GameRandom::next(int minimum, int maximum) {
    if (minimum > maximum) { throw std::invalid_argument("minimum must not exceed maximum"); }
    return static_cast<int>(util::gRandom(minimum, maximum));
}

}
```

Replace the temporary interface target in `src/CMakeLists.txt`:

```cmake
file(GLOB BATTLE_FILES battle/*.cc battle/*.hh)
add_library(hojy_battle STATIC ${BATTLE_FILES})
set_target_properties(hojy_battle PROPERTIES CXX_STANDARD 17)
target_include_directories(hojy_battle PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
```

- [ ] **Step 4: Run the focused and full test targets**

```powershell
cmake --build cmake-build-battle --target battle_random_tests -j 4
ctest --test-dir cmake-build-battle --output-on-failure -R battle_random_tests
```

Expected: `100% tests passed, 0 tests failed`.

- [ ] **Step 5: Commit deterministic randomness**

```powershell
git add CMakeLists.txt src/CMakeLists.txt src/battle/random.hh src/battle/random.cc src/battle/game_random.hh src/battle/game_random.cc tests/CMakeLists.txt tests/test_support.hh tests/battle/random_tests.cc
git commit -m "feat(battle): add deterministic random source"
```

### Task 3: Add the pure reverse-engineering manifest module

**Files:**
- Create: `tools/reverse/manifest.py`
- Create: `tests/reverse/test_manifest.py`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing Python tests**

Create `tests/reverse/test_manifest.py`:

```python
import json
import tempfile
import unittest
from pathlib import Path

from manifest import BinaryRecord, EvidenceRecord, Manifest, sha256_file


class ManifestTests(unittest.TestCase):
    def test_sha256_file_returns_lowercase_digest(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sample.bin"
            path.write_bytes(b"abc")
            self.assertEqual(
                sha256_file(path),
                "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            )

    def test_json_order_is_stable(self):
        manifest = Manifest(source_root="D:/DOS/LEGEND")
        manifest.add_binary(BinaryRecord("Z.DAT", 343217, "b" * 64))
        manifest.add_binary(BinaryRecord("Z.COM", 413, "a" * 64))
        manifest.add_evidence(EvidenceRecord(
            "DATA-WAR-LOAD", "Z.DAT", "0x31DA0", "function",
            "Loads WAR.STA", "confirmed"))
        manifest.add_evidence(EvidenceRecord(
            "ENTRY-ZCOM-EXEC", "Z.COM", "0x10195", "process-exec",
            "Executes Z.DAT", "confirmed"))

        data = json.loads(manifest.to_json())

        self.assertEqual([item["path"] for item in data["binaries"]], ["Z.COM", "Z.DAT"])
        self.assertEqual(
            [item["evidence_id"] for item in data["evidence"]],
            ["DATA-WAR-LOAD", "ENTRY-ZCOM-EXEC"],
        )

    def test_duplicate_evidence_is_rejected(self):
        manifest = Manifest(source_root="D:/DOS/LEGEND")
        evidence = EvidenceRecord(
            "ENTRY-ZCOM-EXEC", "Z.COM", "0x10195", "process-exec",
            "Executes Z.DAT", "confirmed")
        manifest.add_evidence(evidence)
        with self.assertRaisesRegex(ValueError, "duplicate evidence id"):
            manifest.add_evidence(evidence)


if __name__ == "__main__":
    unittest.main()
```

Run:

```powershell
python -m unittest tests/reverse/test_manifest.py -v
```

Expected: import failure because `manifest.py` does not exist.

- [ ] **Step 2: Implement immutable records and stable JSON**

Create `tools/reverse/manifest.py`:

```python
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
        if any((item.parent, item.child, item.kind, item.address) == key
               for item in self.dependencies):
            raise ValueError(f"duplicate dependency: {record.parent} -> {record.child}")
        self.dependencies.append(record)

    def add_evidence(self, record: EvidenceRecord) -> None:
        if any(item.evidence_id == record.evidence_id for item in self.evidence):
            raise ValueError(f"duplicate evidence id: {record.evidence_id}")
        self.evidence.append(record)

    def to_dict(self) -> dict[str, object]:
        return {
            "source_root": self.source_root,
            "binaries": [asdict(item) for item in sorted(self.binaries, key=lambda item: item.path)],
            "dependencies": [asdict(item) for item in sorted(
                self.dependencies,
                key=lambda item: (item.parent, item.child, item.kind, item.address),
            )],
            "evidence": [asdict(item) for item in sorted(
                self.evidence, key=lambda item: item.evidence_id)],
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
```

- [ ] **Step 3: Register the Python test in CTest**

Add to `tests/CMakeLists.txt`:

```cmake
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_test(
    NAME reverse_manifest_tests
    COMMAND ${Python3_EXECUTABLE} -m unittest tests/reverse/test_manifest.py -v)
set_tests_properties(reverse_manifest_tests PROPERTIES
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    ENVIRONMENT "PYTHONPATH=${PROJECT_SOURCE_DIR}/tools/reverse")
```

- [ ] **Step 4: Verify Python and CTest execution**

```powershell
python -m unittest tests/reverse/test_manifest.py -v
ctest --test-dir cmake-build-battle --output-on-failure -R reverse_manifest_tests
```

Expected: all manifest tests pass.

- [ ] **Step 5: Commit the manifest module**

```powershell
git add tools/reverse/manifest.py tests/reverse/test_manifest.py tests/CMakeLists.txt
git commit -m "feat(reverse): add deterministic evidence manifest"
```

### Task 4: Add CLI validation without opening IDA

**Files:**
- Create: `tools/reverse/extract_battle_evidence.py`
- Create: `tests/reverse/test_extract_cli.py`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write failing CLI tests**

Test `--help`, missing `Z.COM`, missing `Z.DAT`, and expected-hash mismatch through a `--validate-only` mode. The CLI contract is:

```powershell
python tools/reverse/extract_battle_evidence.py `
  --game-dir D:/DOS/LEGEND `
  --output docs/reverse/manifests/legend-z-baseline.json `
  --validate-only
```

Expected exit codes:

- `0`: required files exist and match configured hashes.
- `2`: argument or file validation error.
- `3`: binary hash mismatch.

- [ ] **Step 2: Run the test and verify failure**

```powershell
python -m unittest tests/reverse/test_extract_cli.py -v
```

Expected: failure because the CLI does not exist.

- [ ] **Step 3: Implement argument parsing and validation**

The module must import only standard-library and `manifest` modules until validation completes. Define constants for the approved `Z.COM` and `Z.DAT` hashes. Print validation failures to stderr and never write an output file in `--validate-only` mode.

- [ ] **Step 4: Register and run the CLI tests**

Register `reverse_extract_cli_tests` in CTest with the same `PYTHONPATH`. Run:

```powershell
python -m unittest tests/reverse/test_extract_cli.py -v
ctest --test-dir cmake-build-battle --output-on-failure -R reverse_extract_cli_tests
```

Expected: all CLI tests pass.

- [ ] **Step 5: Commit validation behavior**

```powershell
git add tools/reverse/extract_battle_evidence.py tests/reverse/test_extract_cli.py tests/CMakeLists.txt
git commit -m "feat(reverse): validate original battle binaries"
```

### Task 5: Implement the idalib backend

**Files:**
- Create: `tools/reverse/ida_backend.py`
- Create: `tests/reverse/test_ida_backend.py`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1: Write an optional integration test against the approved binaries**

The test must skip unless `HOJY_ORIGINAL_DATA_DIR` is set. When enabled, assert:

- `Z.COM` processor is `metapc`, has one segment and three functions.
- `Z.DAT` processor is `metapc`, has two segments and at least 570 functions.
- exact string `war.sta` resolves to address `0x58A3C`.
- its code reference belongs to function `0x31DA0`.

Run with the environment variable set:

```powershell
$env:HOJY_ORIGINAL_DATA_DIR='D:/DOS/LEGEND'
python -m unittest tests/reverse/test_ida_backend.py -v
```

Expected: import failure because `ida_backend.py` does not exist.

- [ ] **Step 2: Implement a database context manager**

`ida_backend.py` must import `idapro` before all IDA modules and provide:

```python
class IdaDatabase:
    def __init__(self, path: Path): ...
    def __enter__(self) -> "IdaDatabase": ...
    def __exit__(self, exc_type, exc, traceback) -> None: ...
    def processor_name(self) -> str: ...
    def segment_count(self) -> int: ...
    def function_count(self) -> int: ...
    def find_exact_string(self, value: str) -> int: ...
    def xrefs_to(self, address: int) -> list[int]: ...
    def containing_function(self, address: int) -> dict[str, object]: ...
    def decompile_summary(self, address: int, max_lines: int = 12) -> list[str]: ...
```

`__exit__` must call `idapro.close_database(False)` so source directories remain unchanged.

- [ ] **Step 3: Register the optional test**

Add `reverse_ida_backend_tests` to CTest and set `HOJY_ORIGINAL_DATA_DIR` only when it is already present in the configure environment. Without the variable, the Python test reports `skipped` and exits successfully.

- [ ] **Step 4: Run the enabled integration test**

```powershell
$env:HOJY_ORIGINAL_DATA_DIR='D:/DOS/LEGEND'
python -m unittest tests/reverse/test_ida_backend.py -v
```

Expected: all enabled assertions pass and no `.idb`/`.i64` file appears in `D:/DOS/LEGEND`.

- [ ] **Step 5: Commit the idalib adapter**

```powershell
git add tools/reverse/ida_backend.py tests/reverse/test_ida_backend.py tests/CMakeLists.txt
git commit -m "feat(reverse): add idalib analysis backend"
```

### Task 6: Extract entry dependencies and battle anchors

**Files:**
- Modify: `tools/reverse/extract_battle_evidence.py`
- Modify: `tests/reverse/test_extract_cli.py`
- Create: `docs/reverse/manifests/legend-z-baseline.json`

- [ ] **Step 1: Write a failing enabled extraction test**

When `HOJY_ORIGINAL_DATA_DIR` is set, run the CLI into a temporary output file and assert these evidence IDs and addresses:

```text
ENTRY-ZCOM-EXEC        Z.COM  0x10195  process-exec
DATA-WAR-LOAD          Z.DAT  0x31DA0  function
ANIM-FIGHT-LOAD        Z.DAT  0x3859E  function
```

Also assert that the manifest dependency list contains `Z.COM -> Z.DAT` and that every binary record contains size and SHA-256.

- [ ] **Step 2: Run the test and verify missing extraction behavior**

```powershell
$env:HOJY_ORIGINAL_DATA_DIR='D:/DOS/LEGEND'
python -m unittest tests/reverse/test_extract_cli.py -v
```

Expected: failure because the full extraction path is not implemented.

- [ ] **Step 3: Implement extraction**

Open `Z.COM` and `Z.DAT` sequentially. Resolve exact strings and their cross-references, capture containing function start/end/name, and store at most 12 normalized pseudocode lines for each approved anchor. Add dependency evidence for the `Z.DAT` string referenced by the DOS exec call. Do not follow unrelated sound or setup executables during this task.

- [ ] **Step 4: Generate the committed baseline manifest**

```powershell
python tools/reverse/extract_battle_evidence.py `
  --game-dir D:/DOS/LEGEND `
  --output docs/reverse/manifests/legend-z-baseline.json
```

Expected: output JSON contains the two approved binaries, one dependency edge and the three required evidence records.

- [ ] **Step 5: Re-run extraction and verify stable output**

```powershell
$first=(Get-FileHash docs/reverse/manifests/legend-z-baseline.json -Algorithm SHA256).Hash
python tools/reverse/extract_battle_evidence.py --game-dir D:/DOS/LEGEND --output docs/reverse/manifests/legend-z-baseline.json
$second=(Get-FileHash docs/reverse/manifests/legend-z-baseline.json -Algorithm SHA256).Hash
if($first -ne $second){ throw 'manifest output is not deterministic' }
```

Expected: hashes are equal.

- [ ] **Step 6: Commit the extraction baseline**

```powershell
git add tools/reverse/extract_battle_evidence.py tests/reverse/test_extract_cli.py docs/reverse/manifests/legend-z-baseline.json
git commit -m "feat(reverse): extract original battle anchors"
```

### Task 7: Document evidence and the behavior matrix

**Files:**
- Create: `docs/reverse/battle-evidence.md`
- Create: `docs/reverse/battle-behavior-matrix.md`

- [ ] **Step 1: Write the evidence document from the generated manifest**

Document the approved hashes, `Z.COM -> Z.DAT` execution relationship, initial IDA counts, three evidence anchors, regeneration command and evidence-status definitions. Include a warning that addresses are valid only for the recorded hashes.

- [ ] **Step 2: Create the behavior matrix with explicit evidence states**

Add rows for:

- turn order and speed ties;
- poison-at-turn-start;
- movement step count and blocking;
- four skill area types;
- real attack and defense;
- MP-limited skill-level downgrade;
- damage randomness and distance decay;
- drain MP, poison, heal, depoison, throwing and rest;
- double attack and skill growth;
- item-use priority;
- AI candidate generation, score and tie breaking;
- death cleanup, victory and defeat;
- experience, level-up, training and item creation.

Every row must use one of 「已确认」「推定」「待运行验证」「修正版差异」 and include either an evidence ID or a concrete next analysis action.

- [ ] **Step 3: Run the document self-check**

```powershell
rg -n 'TBD|TODO|待定|“|”|你|您|同学' docs/reverse
git diff --check
```

Expected: no matches and no whitespace errors.

- [ ] **Step 4: Commit reverse-engineering documentation**

```powershell
git add docs/reverse/battle-evidence.md docs/reverse/battle-behavior-matrix.md
git commit -m "docs(reverse): add battle evidence baseline"
```

### Task 8: Verify the complete baseline

**Files:**
- Modify only if verification exposes a defect in files created by Tasks 1–7.

- [ ] **Step 1: Configure a clean test build**

```powershell
cmake -S . -B cmake-build-battle -G Ninja `
  -DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/c++.exe `
  -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
```

Expected: configure completes without warnings from project-owned CMake files.

- [ ] **Step 2: Build the application and test targets**

```powershell
cmake --build cmake-build-battle --target HeroesOfJinYongMain battle_random_tests -j 4
```

Expected: both targets build successfully.

- [ ] **Step 3: Run all tests with original data enabled**

```powershell
$env:HOJY_ORIGINAL_DATA_DIR='D:/DOS/LEGEND'
ctest --test-dir cmake-build-battle --output-on-failure
```

Expected: all C++ and Python tests pass.

- [ ] **Step 4: Verify repository and source-directory cleanliness**

```powershell
git diff --check
Get-ChildItem D:/DOS/LEGEND -File -Include *.idb,*.i64,*.nam,*.til
git status --short
```

Expected: no IDA database artifacts under the original game directory; Git status contains only intentional plan-tracking edits, if any.

- [ ] **Step 5: Record the stage result**

Update this plan’s checkboxes and append the exact CTest summary and generated manifest SHA-256 under a short 「Execution Result」 section. Commit only the plan update:

```powershell
git add docs/superpowers/plans/2026-08-02-battle-reverse-contract-baseline.md
git commit -m "docs(battle): record reverse baseline completion"
```
