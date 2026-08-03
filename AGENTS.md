# 项目协作规则

本文件适用于整个仓库。修改子目录时，如存在更具体的 `AGENTS.md`，以更具体的规则为准。

## 工程约定

- 项目使用 C++17 和 CMake，需保持 GCC 8+、Clang 7+、MSVC 2019+ 兼容性。
- 源码格式遵循 `src/.clang-format`：4 空格缩进、禁用 Tab、右侧指针对齐、不限制行宽。
- 不修改 `deps/` 中的第三方代码，除非任务明确要求升级或修补依赖。
- 搜索文件和文本时优先使用 `rg` 与 `rg --files`。
- 工作区可能包含其他未提交修改；不得覆盖、还原或删除不属于当前任务的改动。

## Windows UCRT64 环境

MSYS2 安装在 `C:\msys64`。执行 CMake、编译产物或 CTest 前，必须将 UCRT64 运行库目录放到 PATH 最前面：

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
```

缺少该设置时，MinGW 可执行文件可能无诊断退出，测试进程也可能停留在不可见的 DLL 错误对话框中。

常用验证命令：

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build cmake-build-debug --config Debug
ctest --test-dir cmake-build-debug -C Debug --output-on-failure
cmake --build cmake-build-release --config Release
ctest --test-dir cmake-build-release -C Release --output-on-failure
git diff --check
```

首次创建构建目录时启用测试：

```powershell
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
```

`src/CMakeLists.txt` 使用 `file(GLOB ...)` 收集源码。新增 `*.cc` 或 `*.hh` 后，需要重新运行 CMake 配置，再执行构建。

## 实现要求

- 资源、存档和二进制数据加载必须校验文件长度、元素对齐、偏移范围、短读及整数溢出。
- 解析外部数据时先写入临时对象；全部验证成功后再更新全局或现有状态，失败时保留原状态。
- 修改 IDX/GRP、存档、事件或战场格式时，需兼容原版数据约定，并为损坏输入和失败回滚补充测试。
- SDL 音频长度进入 `int` API 前必须检查范围；检查 SDL 转换、分配和设备初始化的返回值。
- 音频回调共享的数据必须使用同一互斥量保护；重新初始化设备时清理依赖旧设备格式的声道和缓存。
- 必需配置或核心资源加载失败时应返回失败状态，避免进入窗口和场景初始化。

## 测试与提交

- 修复缺陷时添加覆盖原始失败条件的回归测试，并验证失败路径不会污染既有状态。
- 至少运行受影响的测试；涉及公共加载、启动、音频或构建逻辑时，运行 Debug 和 Release 全量测试。
- Release 构建可能出现来自 `libADLMIDI`、`fmt` 或既有场景代码的 LTO/ODR 警告。不得把既有第三方警告误判为当前修改，但新增警告需要处理。
- 提交前检查 `git status --short`、`git diff --check` 和暂存差异，避免提交构建产物、凭据或无关文件。
- 提交信息采用 Conventional Commits，使用简短的祈使句描述实际修改。
