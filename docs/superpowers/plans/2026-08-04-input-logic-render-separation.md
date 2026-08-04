# 输入、逻辑与渲染严格隔离实施计划

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将所有输入副作用延后到 fixed logic 阶段，将表现缓存准备与只读绘制分开，并用多态输入消费、命令对象和架构门禁阻止依赖方向回退。

**Architecture:** `core::InputEvent` 位于 `app` 与 `scene` 之下，`app::SdlInputCollector` 继续产生稳定排序的事件；`scene/logic` 只生成并保存多态 `SceneInputIntent`、逻辑控制器和场景命令，不依赖 SDL、`Renderer` 或 `Texture`。`Window::updateFixed()` 在逻辑阶段按事件逐个投递给当前焦点节点。`Node` 同时提供 `InputConsumer`、逻辑更新、表现准备和只读绘制契约；地图、战场和缓存节点把渲染期写操作迁到 `prepareRender()`，逻辑只保存资源 ID，表现层解析纹理。

**Tech Stack:** C++17、CMake、CTest、Python `unittest`、SDL2、现有 `hojy_app`/`hojy_scene`/`hojy_battle` 目标。

---

## 文件结构

- `src/core/input_event.hh`：`InputDevice`、`InputAction` 和跨 `app`/`scene` 的 `InputEvent` 值类型。
- `src/scene/logic/input.hh/.cc`：场景输入键、意图基类、输入消费者、输入端口和稳定 FIFO 实现；不包含 SDL、`Renderer` 或 `Texture`。
- `src/scene/logic/command.hh/.cc`：场景命令基类、函数命令适配器和 generation 命令队列。
- `src/scene/logic/CMakeLists.txt`（若采用独立清单）：纯逻辑源文件与依赖边界说明。
- `src/scene/node.hh/.cc`：输入意图的多态焦点投递、逻辑更新、表现准备和只读渲染遍历。
- `src/scene/window.hh`、`window_input.cc`、`window.cc`：输入只入队；fixed update 消费输入并执行命令屏障；render 只执行 prepare/render。
- `src/scene/warfield_input_mode.hh/.cc`：战场输入模式多态层，替代 `Warfield::handleKeyInput()` 的阶段分支。
- `src/scene/map*.cc`、`globalmap.cc`、`submap.cc`、`warfield_render.cc`：将缓存构建移入 `prepareRender()`，逻辑纹理选择改为资源 ID。
- `tests/scene/input_port_tests.cc`、`command_queue_tests.cc`、`node_phase_tests.cc`、`warfield_input_mode_tests.cc`：行为回归。
- `tests/architecture/input_logic_boundary_tests.py`、`logic_render_boundary_tests.py`、`scene_command_boundary_tests.py`：依赖方向门禁。

## Task 0：下移跨层输入值类型并建立纯逻辑目标

**Files:**
- Create: `src/core/input_event.hh`
- Modify: `src/app/input.hh`
- Modify: `src/app/input.cc`
- Modify: `src/CMakeLists.txt`
- Create: `tests/app/input_event_contract_tests.cc`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1：写失败测试，验证 app 与 scene 可共享无 SDL 输入值**

```cpp
#include "core/input_event.hh"
#include "test_support.hh"

int main() {
    hojy::core::InputEvent event{123, hojy::core::InputDevice::Keyboard,
                                 hojy::core::InputAction::Left};
    HOJY_CHECK_EQ(event.timestamp, 123ULL);
    HOJY_CHECK_EQ(event.action, hojy::core::InputAction::Left);
    return 0;
}
```

- [ ] **Step 2：运行测试确认头文件缺失**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build cmake-build-debug --target input_event_contract_tests
```

Expected: 缺少 `core/input_event.hh`。

- [ ] **Step 3：移动值类型并保持 app API 兼容**

把 `InputDevice`、`InputAction`、`InputEvent` 移到 `hojy::core`；`src/app/input.hh` 使用 `using core::InputDevice`、`using core::InputAction`、`using core::InputEvent` 保留既有调用方。`InputQueue` 继续位于 `hojy::app`。新增 `hojy_scene_logic` 静态库，源文件只来自 `src/scene/logic/*.cc/*.hh`，不链接 SDL；`hojy_scene` 链接它，`hojy_app` 仍链接 `hojy_scene`。

- [ ] **Step 4：运行目标边界测试**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build cmake-build-debug --target input_event_contract_tests
ctest --test-dir cmake-build-debug -R input_event_contract_tests --output-on-failure
```

Expected: `1/1` passed。

- [ ] **Step 5：提交纯值类型边界**

```powershell
git add src/core/input_event.hh src/app/input.hh src/app/input.cc src/CMakeLists.txt tests/app/input_event_contract_tests.cc tests/CMakeLists.txt
git commit -m "refactor(app): move input values below scene"
```

## Task 1：建立多态输入意图与端口

**Files:**
- Create: `src/scene/logic/input.hh`
- Create: `src/scene/logic/input.cc`
- Create: `tests/scene/input_port_tests.cc`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1：写失败测试，定义期望接口与 FIFO 语义**

```cpp
#include "scene/logic/input.hh"
#include "test_support.hh"

#include <iostream>
#include <vector>

namespace {

class RecordingConsumer final : public hojy::scene::InputConsumer {
public:
    void consume(const hojy::scene::KeyIntent &intent) override {
        keys.push_back(intent.key());
    }

    void consume(const hojy::scene::TextIntent &intent) override {
        texts.push_back(intent.text());
    }

    std::vector<hojy::scene::InputKey> keys;
    std::vector<std::wstring> texts;
};

void testQueuedInputPortMapsAndPreservesOrder() {
    hojy::scene::QueuedInputPort port;
    port.enqueue(std::make_unique<hojy::scene::KeyIntent>(
        hojy::scene::InputKey::Left));
    port.enqueue(std::make_unique<hojy::scene::TextIntent>(L"令狐冲"));

    RecordingConsumer consumer;
    HOJY_CHECK_EQ(port.size(), 2U);
    port.deliverNext(consumer);
    port.deliverNext(consumer);

    HOJY_CHECK_EQ(consumer.keys.at(0), hojy::scene::InputKey::Left);
    HOJY_CHECK_EQ(consumer.texts.at(0), L"令狐冲");
    HOJY_CHECK_EQ(port.empty(), true);
}

}

int main() {
    try {
        testQueuedInputPortMapsAndPreservesOrder();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
```

- [ ] **Step 2：配置并运行测试，确认因接口缺失失败**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build cmake-build-debug --target scene_input_port_tests
```

Expected: 编译失败，提示缺少 `scene/logic/input.hh` 或 `QueuedInputPort`。

- [ ] **Step 3：实现输入意图和端口**

```cpp
// src/scene/logic/input.hh
#pragma once

#include <cstddef>
#include <deque>
#include <memory>
#include <string>

namespace hojy::scene {

enum class InputKey {
    None,
    Up,
    Down,
    Left,
    Right,
    Accept,
    Cancel,
    Space,
    Backspace,
};

class KeyIntent;
class TextIntent;

class InputConsumer {
public:
    virtual ~InputConsumer() = default;
    virtual void consume(const KeyIntent &intent) = 0;
    virtual void consume(const TextIntent &intent) = 0;
};

class SceneInputIntent {
public:
    virtual ~SceneInputIntent() = default;
    virtual void deliver(InputConsumer &consumer) const = 0;
};

class KeyIntent final : public SceneInputIntent {
public:
    explicit KeyIntent(InputKey key): key_(key) {}
    [[nodiscard]] InputKey key() const { return key_; }
    void deliver(InputConsumer &consumer) const override { consumer.consume(*this); }

private:
    InputKey key_;
};

class TextIntent final : public SceneInputIntent {
public:
    explicit TextIntent(std::wstring text): text_(std::move(text)) {}
    [[nodiscard]] const std::wstring &text() const { return text_; }
    void deliver(InputConsumer &consumer) const override { consumer.consume(*this); }

private:
    std::wstring text_;
};

class InputPort {
public:
    virtual ~InputPort() = default;
    virtual void enqueue(std::unique_ptr<SceneInputIntent> intent) = 0;
};

class QueuedInputPort final : public InputPort {
public:
    void enqueue(std::unique_ptr<SceneInputIntent> intent) override;
    [[nodiscard]] bool empty() const { return intents_.empty(); }
    [[nodiscard]] std::size_t size() const { return intents_.size(); }
    bool deliverNext(InputConsumer &consumer);

private:
    std::deque<std::unique_ptr<SceneInputIntent>> intents_;
};

}
```

`src/scene/logic/input.cc` 不直接依赖 `app::InputEvent`；端口只提供 `enqueue(std::unique_ptr<SceneInputIntent>)`，由 `window_input.cc` 把 `core::InputEvent` 映射为 `KeyIntent` 或 `TextIntent`。`Quit` 不进入场景端口。`deliverNext()` 先移动队首智能指针，再调用 `deliver()`，确保消费期间追加的新输入不会破坏当前对象生命周期；意图在构造时保存原始 `timestamp`、`device` 和 `sequence` 元数据。

- [ ] **Step 4：运行输入端口测试并确认通过**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build cmake-build-debug --target scene_input_port_tests
ctest --test-dir cmake-build-debug -R scene_input_port_tests --output-on-failure
```

Expected: `1/1` passed。

- [ ] **Step 5：提交输入契约**

```powershell
git add src/scene/logic/input.hh src/scene/logic/input.cc tests/scene/input_port_tests.cc tests/CMakeLists.txt
git commit -m "refactor(input): add polymorphic scene intents"
```

## Task 2：让输入只入队并在 fixed logic 中消费

**Files:**
- Modify: `src/app/application.cc`
- Modify: `src/scene/window.hh`
- Modify: `src/scene/window_input.cc`
- Modify: `src/scene/window.cc`
- Modify: `src/scene/node.hh`
- Modify: `src/scene/node.cc`
- Create: `tests/scene/node_phase_tests.cc`
- Modify: `tests/scene/node_lifetime_tests.cc`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1：写失败测试，证明入队阶段不执行逻辑**

新增 `ProbeNode`，其 `consumeKeyIntent()` 增加计数。测试先调用 `QueuedInputPort::enqueue(...)`，断言计数仍为 `0`；随后调用 `deliverNext(root)`，断言计数变为 `1`。再排入两个意图，第一个创建/删除焦点子节点，应用删除屏障后投递第二个意图，断言第二个意图按更新后的焦点树消费。

- [ ] **Step 2：运行测试，确认旧同步派发行为导致失败**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build cmake-build-debug --target scene_node_phase_tests
ctest --test-dir cmake-build-debug -R scene_node_phase_tests --output-on-failure
```

Expected: 缺少 `dispatchIntent()` 或输入仍在 `dispatchInput()` 中立即执行。

- [ ] **Step 3：实现节点输入消费者契约**

`Node` 继承 `InputConsumer`，新增：

```cpp
void consume(const KeyIntent &intent) override;
void consume(const TextIntent &intent) override;

virtual void consumeKeyIntent(InputKey key) {}
virtual void consumeTextIntent(const std::wstring &text) {}
```

两个 `consume()` 重载保留现有「最后一个活动子节点优先」规则：存在活动子节点时继续调用子节点的同名 `consume()`，到达叶节点后才转发到业务虚函数。业务函数只会由 fixed logic 阶段调用。删除 `doHandleKeyInput()`、`doTextInput()` 及公开的同步输入派发方法。

- [ ] **Step 4：实现 Window 输入队列与逐事件屏障**

`Window` 新增 `QueuedInputPort inputPort_`。`dispatchInput()` 只把 `core::InputEvent` 映射为意图并入队，不访问 `Node`、`map_`、`popup_` 或 `processingStage_`；`Application` 在看到 `InputAction::Quit` 时直接 `window_.requestQuit()`，不把系统退出传给场景。

`Window::updateFixed()` 先执行逐事件子事务：

```cpp
while (!inputPort_.empty()) {
    auto *target = popup_ ? popup_ : map_;
    if (!target) { break; }
    processingStage_ = true;
    inputPort_.deliverNext(*target);
    processingStage_ = false;
    applyDeferredNodes();
    applyDeferredCommands();
}
```

随后再执行节点 `update()`。每个意图之后应用屏障，保持旧实现中同 tick 多事件、焦点变化和 deferred command 的可观察顺序。

- [ ] **Step 5：把所有业务输入覆写改名为逻辑消费函数**

在 `endscreen`、`dead`、`extendednode`、`itemview`、`mapwithevent`、`menu`、`messagebox`、`statusview`、`submap`、`talkbox`、`title`、`warfield` 中，将 `handleKeyInput()` 改为 `consumeKeyIntent()`，将 `handleTextInput()` 改为 `consumeTextIntent()`，参数改为 `InputKey`。保留每个具体类原有行为，不在本步骤重写业务分支。

- [ ] **Step 6：运行节点、输入和应用测试**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build cmake-build-debug --target scene_node_phase_tests scene_node_lifetime_tests application_contract_tests
ctest --test-dir cmake-build-debug -R "scene_node_(phase|lifetime)_tests|application_contract_tests" --output-on-failure
```

Expected: 全部通过。

- [ ] **Step 7：提交 fixed logic 输入消费**

```powershell
git add src/app/application.cc src/scene tests/scene/node_phase_tests.cc tests/scene/node_lifetime_tests.cc tests/CMakeLists.txt
git commit -m "refactor(input): consume intents during fixed logic"
```

## Task 3：引入多态场景命令队列

**Files:**
- Create: `src/scene/command.hh`
- Create: `src/scene/command.cc`
- Create: `tests/scene/command_queue_tests.cc`
- Modify: `src/scene/window.hh`
- Modify: `src/scene/window.cc`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1：写失败测试，覆盖 FIFO、重入和异常保留**

测试定义两个 `SceneCommand` 子类。第一个执行时追加第三个命令；一次 `executeGeneration()` 只执行原始批次，第二次才执行第三个。另一个命令抛出异常，测试要求未执行的命令仍留在队列中，避免静默丢失。

- [ ] **Step 2：运行测试并确认接口缺失**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build cmake-build-debug --target scene_command_queue_tests
```

Expected: 缺少 `scene/command.hh`。

- [ ] **Step 3：实现命令模式**

```cpp
class SceneCommand {
public:
    virtual ~SceneCommand() = default;
    virtual void execute() = 0;
};

class FunctionSceneCommand final : public SceneCommand {
public:
    explicit FunctionSceneCommand(std::function<void()> function)
        : function_(std::move(function)) {}
    void execute() override { if (function_) { function_(); } }

private:
    std::function<void()> function_;
};

class SceneCommandQueue final {
public:
    void push(std::unique_ptr<SceneCommand> command);
    void push(std::function<void()> function);
    void executeGeneration();
    void executeAllGenerations();
    [[nodiscard]] bool empty() const { return commands_.empty(); }

private:
    std::deque<std::unique_ptr<SceneCommand>> commands_;
};
```

`executeGeneration()` 移动当前队列到局部批次；执行失败时把当前未执行命令按原顺序放回队首，再抛出异常。执行期间追加的命令留在下一 generation；`executeAllGenerations()` 在当前事件屏障内逐 generation 执行，直到进入屏障时已存在的命令全部完成，防止下一个输入看到半完成焦点状态。

- [ ] **Step 4：用命令队列替换 Window 的裸函数队列**

`Window::defer(std::function<void()>)` 保持公共兼容入口，但内部构造 `FunctionSceneCommand`。`deferredCommands_` 改为 `SceneCommandQueue`，`applyDeferredCommands()` 只负责重入保护并调用 `executeAllGenerations()`；单个 generation 内执行期间追加的新命令必须留到下一 generation，事件屏障返回前则继续执行所有已产生 generation。

- [ ] **Step 5：运行命令、节点和窗口测试**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build cmake-build-debug --target scene_command_queue_tests scene_node_phase_tests
ctest --test-dir cmake-build-debug -R "scene_(command_queue|node_phase)_tests" --output-on-failure
```

Expected: 全部通过。

- [ ] **Step 6：提交命令模式**

```powershell
git add src/scene/command.hh src/scene/command.cc src/scene/window.hh src/scene/window.cc tests/scene/command_queue_tests.cc tests/CMakeLists.txt
git commit -m "refactor(scene): execute deferred work as commands"
```

## Task 4：用多态模式隔离战场输入消费

**Files:**
- Create: `src/scene/warfield_input_mode.hh`
- Create: `src/scene/warfield_input_mode.cc`
- Modify: `src/scene/warfield.hh`
- Modify: `src/scene/warfield_input.cc`
- Modify: `src/scene/warfield_ui.cc`
- Modify: `src/scene/warfield_turns.cc`
- Modify: `src/scene/warfield_actions.cc`
- Create: `tests/scene/warfield_input_mode_tests.cc`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1：写失败测试，定义输入模式多态行为**

测试使用独立的 `WarfieldInputContext` 假对象验证：`PassiveWarfieldInputMode` 只响应取消自动控制；`MoveSelectingInputMode` 在确认时提交移动目标；`AttackSelectingInputMode` 在确认时提交攻击目标；取消时两种选择模式都调用统一撤销操作。

- [ ] **Step 2：运行测试并确认模式类型缺失**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build cmake-build-debug --target scene_warfield_input_mode_tests
```

Expected: 缺少 `warfield_input_mode.hh`。

- [ ] **Step 3：实现 OOP 输入模式**

定义 `WarfieldInputContext` 抽象接口，提供 `moveCursor()`、`confirmMove()`、`confirmAttack()`、`cancelSelection()` 和 `cancelAutoControl()`。定义 `WarfieldInputMode` 基类及三个具体类；各具体类通过虚函数调用上下文，不访问 `Warfield` 字段，也不包含阶段分支。

- [ ] **Step 4：让 Warfield 实现上下文并集中阶段切换**

`Warfield` 实现 `WarfieldInputContext`，持有 `std::unique_ptr<WarfieldInputMode> inputMode_`。新增 `setStage(Stage)`，设置 `stage_` 后根据目标阶段从工厂创建对应模式。把所有 `stage_ = ...` 改为 `setStage(...)`；`consumeKeyIntent()` 只调用 `inputMode_->consume(*this, key)`。

工厂中的阶段选择只在状态转换时执行一次；每个输入事件不再检查 `stage_`。

- [ ] **Step 5：运行战场输入与既有战斗测试**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build cmake-build-debug --target scene_warfield_input_mode_tests battle_engine_tests battle_movement_tests
ctest --test-dir cmake-build-debug -R "scene_warfield_input_mode_tests|battle_(engine|movement)_tests" --output-on-failure
```

Expected: 全部通过。

- [ ] **Step 6：提交战场输入模式**

```powershell
git add src/scene/warfield* tests/scene/warfield_input_mode_tests.cc tests/CMakeLists.txt
git commit -m "refactor(battle): model input states as polymorphic modes"
```

## Task 5：建立表现准备阶段并让绘制遍历只读

**Files:**
- Modify: `src/scene/node.hh`
- Modify: `src/scene/node.cc`
- Modify: `src/scene/window.cc`
- Modify: `src/scene/nodewithcache.hh`
- Modify: `src/scene/nodewithcache.cc`
- Modify: all `src/scene/*.hh` render declarations
- Modify: all `src/scene/*.cc` render definitions
- Create: `tests/scene/render_phase_tests.cc`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1：写失败测试，证明 prepare 与 render 分离**

测试节点在 `prepareRender()` 中增加 prepare 计数，在 `render() const` 中只增加外部记录器计数。调用 `dispatchRender()` 前断言缓存未准备；调用 `dispatchPrepareRender()` 后缓存准备一次；连续调用两次 `dispatchRender()` 不改变节点逻辑快照和删除队列。

- [ ] **Step 2：运行测试并确认接口缺失**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build cmake-build-debug --target scene_render_phase_tests
```

Expected: 缺少 `prepareRender()` 或 `render() const`。

- [ ] **Step 3：修改 Node 契约**

`Node` 新增 `dispatchPrepareRender()`、`doPrepareRender()` 和虚函数 `prepareRender()`；把 `render()`、`doRender()`、`dispatchRender()` 改为 `const`。渲染遍历不再修改 `dispatchDepth_`，因为渲染期命令和删除由门禁禁止；子节点通过只读快照遍历。

- [ ] **Step 4：迁移 NodeWithCache**

`NodeWithCache::update()` 不再调用 `rebuildCache()`；`prepareRender()` 调用 `rebuildCache()`。`forceUpdate()` 只标记 dirty 并由显式 `prepareRender()` 构建；需要同步尺寸的创建路径改为先调用 `dispatchPrepareRender()`，再 `makeCenter()`。`render() const` 只提交现有 texture。

- [ ] **Step 5：批量修正所有 render 覆写为 const**

更新 `CharListMenu`、`EndScreen`、`GlobalMap`、`Map`、`Mask`、`NodeWithCache`、`SubMap`、`Warfield` 及其他直接覆写。只做签名和只读调用修正，不在本步骤迁移缓存代码。

- [ ] **Step 6：Window 调用 prepare/render，且不切换逻辑 processing 标志**

`Window::render()` 先调用活动根节点的 `doPrepareRender()`，再调用 `doRender()`；删除对 `processingStage_` 的写入。渲染阶段不执行 deferred nodes 或 commands。

- [ ] **Step 7：运行节点、缓存和架构测试**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build cmake-build-debug --target scene_render_phase_tests scene_node_lifetime_tests
ctest --test-dir cmake-build-debug -R "scene_(render_phase|node_lifetime)_tests|architecture_render_purity_tests" --output-on-failure
```

Expected: 全部通过。

- [ ] **Step 8：提交表现阶段契约**

```powershell
git add src/scene tests/scene/render_phase_tests.cc tests/CMakeLists.txt
git commit -m "refactor(render): separate preparation from drawing"
```

## Task 6：迁移地图和战场渲染缓存写操作

**Files:**
- Modify: `src/scene/map.hh`
- Modify: `src/scene/map.cc`
- Modify: `src/scene/globalmap.hh`
- Modify: `src/scene/globalmap.cc`
- Modify: `src/scene/submap.hh`
- Modify: `src/scene/submap.cc`
- Modify: `src/scene/warfield.hh`
- Modify: `src/scene/warfield_render.cc`
- Modify: `tests/architecture/render_purity_tests.py`

- [ ] **Step 1：扩展失败门禁，禁止 render 内提交缓存状态**

新增断言：`GlobalMap::render()`、`SubMap::render()`、`Warfield::render()` 不含 `drawDirty_ =`、`lock(`、`unlock(`、`memset(`；`Map::renderMiniPanel()` 不含 `miniPanelDirty_ =`、`setTargetTexture(`。测试还要求对应类存在 `prepareRender()` 或 `prepareMiniPanel()`。

- [ ] **Step 2：运行门禁并确认现状失败**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
python -m unittest tests/architecture/render_purity_tests.py -v
```

Expected: 地图、战场 render 仍重建缓存而失败。

- [ ] **Step 3：拆分 Map 小地图缓存**

把 `showMiniPanel()` 拆成 `prepareMiniPanel()` 和 `renderMiniPanel() const`。`prepareMiniPanel()` 只在 dirty 时构建 target texture，构建完成后清 dirty 并更新布局；`renderMiniPanel()` 只绘制 texture 和角色位置标记。

- [ ] **Step 4：迁移 GlobalMap terrain 缓存**

把 `render()` 中 `if (drawDirty_)` 的完整块移动到 `prepareRender()`；完成 texture unlock 后才清 `drawDirty_`。`render() const` 只清屏、绘制两层 terrain、角色、云和小地图。

- [ ] **Step 5：迁移 SubMap terrain 缓存**

把 terrain RLE 解码、`charHeight_` 计算和 dirty 提交移到 `prepareRender()`。`render() const` 只绘制缓存、角色和小地图。

- [ ] **Step 6：迁移 Warfield terrain/effect overlay 缓存**

把 effect overlay 计算、terrain texture 锁定和 RLE 解码移到 `prepareRender()`；绘制阶段只使用两个 terrain texture、popup number 值和 status view。修复 status view 的直接 `render()` 调用，改为统一 `dispatchRender()` 或纳入节点树。

- [ ] **Step 7：运行架构门禁和主程序构建**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
python -m unittest tests/architecture/render_purity_tests.py -v
cmake --build cmake-build-debug --target HeroesOfJinYongMain
```

Expected: 门禁通过，Debug 主程序构建成功。

- [ ] **Step 8：提交缓存迁移**

```powershell
git add src/scene/map.* src/scene/globalmap.* src/scene/submap.* src/scene/warfield* tests/architecture/render_purity_tests.py
git commit -m "refactor(render): prepare map and battle caches before draw"
```

## Task 7：移除逻辑阶段的纹理解析和战斗背包绕行

**Files:**
- Modify: `src/scene/mapwithevent.hh`
- Modify: `src/scene/map_navigation.cc`
- Modify: `src/scene/globalmap.hh`
- Modify: `src/scene/globalmap.cc`
- Modify: `src/scene/submap.cc`
- Modify: `src/scene/itemview.hh`
- Modify: `src/scene/itemview.cc`
- Modify: `src/scene/warfield_ui.cc`
- Create: `tests/architecture/logic_render_boundary_tests.py`
- Modify: `tests/architecture/warfield_battle_boundary_tests.py`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1：写失败门禁**

门禁要求 `MapWithEvent` 保存 `mainCharSpriteId_` 而不是 `mainCharTex_`；`GlobalMap::update()` 不调用 `cloudTexMgr_` 或 `getOrLoadTexture()`；`warfield_actions.cc`、`warfield_turns.cc` 和输入消费文件不访问 `renderer_`；战斗 `ItemView` 路径不调用无参数 `world::state::useItem()` 或生产 `gBag`。

- [ ] **Step 2：运行门禁并确认失败**

```powershell
python -m unittest tests/architecture/logic_render_boundary_tests.py tests/architecture/warfield_battle_boundary_tests.py -v
```

Expected: 当前纹理指针和 `ItemView → gBag` 路径被检测到。

- [ ] **Step 3：地图主角和云改存资源 ID**

`MapWithEvent::updateMainCharTexture()` 只设置 `mainCharSpriteId_`；`prepareRender()` 通过 `getOrLoadTexture(mainCharSpriteId_)` 更新表现缓存指针。云逻辑保存 `cloudSpriteId_[3]`，`GlobalMap::prepareRender()` 解析为 `cloud_[3]`。

- [ ] **Step 4：为 ItemView 注入物品使用策略**

定义多态 `ItemUsePolicy`：普通地图策略调用世界背包，战场策略持有 `Warfield` 提供的事务背包/动作提交接口。`ItemView` 只把选择结果传给策略，不检查战斗类型，不直接访问 `gBag`。`Warfield::playerMenu()` 创建战斗策略并交给 `ItemView`。

- [ ] **Step 5：删除逻辑文件中无用 Renderer 访问**

删除 `warfield_actions.cc` 中未使用的 `renderer_->ttf()`；把仍用于表现请求的音效和消息转换为 deferred command 或表现事件，逻辑文件不直接进行绘制。

- [ ] **Step 6：运行门禁、战斗与世界动作测试**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
python -m unittest tests/architecture/logic_render_boundary_tests.py tests/architecture/warfield_battle_boundary_tests.py -v
cmake --build cmake-build-debug --target action_contract_tests battle_engine_tests HeroesOfJinYongMain
ctest --test-dir cmake-build-debug -R "action_contract_tests|battle_engine_tests|architecture_(logic_render|warfield_battle)_boundary_tests" --output-on-failure
```

Expected: 全部通过。

- [ ] **Step 7：提交资源与背包边界**

```powershell
git add src/scene tests/architecture tests/CMakeLists.txt
git commit -m "refactor(scene): isolate logic from render resources"
```

## Task 8：补齐输入、命令和渲染架构门禁

**Files:**
- Create: `tests/architecture/input_logic_boundary_tests.py`
- Create: `tests/architecture/scene_command_boundary_tests.py`
- Modify: `tests/architecture/app_scene_boundary_tests.py`
- Modify: `tests/architecture/render_purity_tests.py`
- Modify: `tests/CMakeLists.txt`

- [ ] **Step 1：写完整门禁并确认会捕获旧模式**

`input_logic_boundary_tests.py` 扫描输入入口，要求 `Window::dispatchInput()` 只把事件映射为意图并调用 `inputPort_.enqueue()`，业务类不再定义 `handleKeyInput()`/`handleTextInput()`，`*_input.cc` 不访问 `gSaveData`、`gBag`、`gWindow` 或 `Renderer`。

`scene_command_boundary_tests.py` 要求 `Window` 持有 `SceneCommandQueue`，`render()` 不调用命令或删除屏障，逻辑更新后存在明确命令屏障。

`render_purity_tests.py` 要求所有 Node `render()` 为 `const`，并禁止 render 函数体中的业务字段赋值、容器修改和生命周期调用。

- [ ] **Step 2：注册门禁到 CTest**

在 `tests/CMakeLists.txt` 添加 `architecture_input_logic_boundary_tests`、`architecture_logic_render_boundary_tests` 和 `architecture_scene_command_boundary_tests`，工作目录设为 `${PROJECT_SOURCE_DIR}`。

- [ ] **Step 3：运行全部架构测试**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
ctest --test-dir cmake-build-debug -R architecture_ --output-on-failure
```

Expected: 全部架构测试通过。

- [ ] **Step 4：提交架构门禁**

```powershell
git add tests/architecture tests/CMakeLists.txt
git commit -m "test(architecture): enforce input logic render boundaries"
```

## Task 9：清理过渡 API、更新文档并完成全量验证

**Files:**
- Modify: `docs/superpowers/specs/2026-08-04-input-logic-render-separation-design.md`
- Modify: affected `src/scene/*.hh` and `*.cc`
- Delete: obsolete input/render compatibility methods and empty translation units

- [ ] **Step 1：扫描过渡符号和反向依赖**

```powershell
rg -n 'handleKeyInput|handleTextInput|doHandleKeyInput|doTextInput|dispatchKeyInput|dispatchTextInput' src tests
rg -n 'drawDirty_\s*=|miniPanelDirty_\s*=|applyDeferredCommands|applyDeferredNodes' src/scene/*render*.cc src/scene/globalmap.cc src/scene/submap.cc src/scene/map.cc
rg -n 'renderer_->|Texture\s*\*' src/scene/warfield_actions.cc src/scene/warfield_turns.cc src/scene/warfield_input.cc
```

Expected: 旧输入符号无匹配；render 函数体无状态提交；战场逻辑文件无渲染依赖。

- [ ] **Step 2：删除空壳和过渡适配器**

删除不再引用的旧方法、字段和文件；若新增或删除 `*.cc`/`*.hh`，重新运行 Debug/Release CMake 配置。

- [ ] **Step 3：更新设计文档的实际落地记录**

在设计文档增加「实际落地」章节，列出最终接口、迁移文件、被删除的旧入口及原因；不保留未完成占位项。

- [ ] **Step 4：运行 Debug 全量构建和测试**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build cmake-build-debug --config Debug
ctest --test-dir cmake-build-debug -C Debug --output-on-failure
```

Expected: 构建退出码 `0`，全部测试通过。

- [ ] **Step 5：运行 Release 全量构建和测试**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake -S . -B cmake-build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build cmake-build-release --config Release
ctest --test-dir cmake-build-release -C Release --output-on-failure
```

Expected: 构建退出码 `0`，全部测试通过；既有第三方 LTO/ODR 警告单独记录，本轮新增警告为 `0`。

- [ ] **Step 6：运行最终差异和工作区检查**

```powershell
git diff --check
git status --short
git diff --cached
```

Expected: `git diff --check` 返回 `0`；未包含构建产物、凭据或原工作区的 `debug*.txt`、`release*.txt`。

- [ ] **Step 7：提交最终清理**

```powershell
git add src tests docs/superpowers/specs/2026-08-04-input-logic-render-separation-design.md
git commit -m "refactor(architecture): enforce strict runtime phase isolation"
```
