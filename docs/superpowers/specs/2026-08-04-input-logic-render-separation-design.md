# 输入、逻辑与渲染严格隔离设计

**状态**：已确认设计

**日期**：2026-08-04

**目标**：在保持原版存档、事件、战场格式和行为时序兼容的前提下，建立可验证的输入、逻辑、渲染契约，消除输入回调直接移动或修改业务状态、逻辑代码直接依赖渲染资源，以及渲染阶段改写逻辑或共享流程状态的问题。

## 1. 背景与现状

现有主循环已经具备「输入采集 → fixed update → compatibility update → render → flush」的阶段顺序，`src/app/sdl_input.cc` 也已从 `scene::Window` 中移出 SDL 事件采集。但阶段内部仍存在跨层调用：

- `src/scene/window_input.cc` 的 `Window::dispatchInput()` 同步调用 `Node::handleKeyInput()`；地图移动、战斗行动、菜单确认和场景转场会在输入阶段直接发生。
- `src/scene/map_navigation.cc`、`src/scene/submap.cc` 和 `src/scene/globalmap.cc` 的移动逻辑会直接修改位置、存档字段、事件状态，并调用窗口、音频和纹理接口。
- `src/scene/warfield_input.cc`、`src/scene/warfield_ui.cc` 和 `src/scene/itemview.cc` 可以从输入路径直接进入战斗状态机或全局 `gBag`。
- `src/scene/globalmap.cc`、`src/scene/submap.cc`、`src/scene/warfield_render.cc` 和 `src/scene/map.cc` 的 `render()` 会清除脏标记、重建纹理、写入布局字段或改变缓存生命周期状态。
- 逻辑对象保存 `Texture*` 并调用 `Renderer`，使地图和战斗逻辑依赖 GPU 资源。
- 现有 `tests/architecture/render_purity_tests.py` 等门禁主要依赖函数体字符串检查，不能发现传递性副作用和跨文件绕行。

因此，本设计采用契约级严格隔离，并按不同输入消费方式使用多态端口和状态对象。暂不要求所有场景一次性物理拆成三棵对象树，但最终业务状态、输入意图和渲染资源必须有明确所有者。

## 2. 目标与非目标

### 2.1 目标

1. 平台输入只产生带时间戳的抽象输入事件；输入端口只把事件转换为类型化意图并入队。
2. 固定逻辑阶段成为地图、事件、战斗、菜单和持久状态的唯一写入口。
3. 副作用通过可执行的类型化场景命令排队，在逻辑阶段结束后的单一屏障执行。
4. 渲染只读取不可变值快照；渲染缓存只能属于表现层，且采用成功后提交的事务式更新。
5. 地图、战斗、菜单和标题等输入消费差异通过多态对象表达，不在公共入口堆叠场景分支。
6. 为输入顺序、逻辑确定性、渲染纯度、失败回滚和跨层依赖建立可重复的测试与架构门禁。
7. 保持原版格式、随机调用顺序、固定逻辑频率和已有存档兼容性。

### 2.2 非目标

- 本设计不重新实现战斗公式、事件 opcode 或资源文件格式。
- 本设计不在第一阶段引入 ECS，也不要求一次性重写所有 `Node` 生命周期代码。
- 本设计不把 GPU 缓存强制复制进世界快照；GPU 资源仍由表现层管理。
- 本设计不改变 `18.2065 Hz` 的 BIOS tick 等效逻辑频率，也不改变 `60 Hz` fixed tick 的外部调度契约。

## 3. 运行时边界

### 3.1 阶段顺序

主循环保持以下顺序，且每个箭头都表示单向数据流：

```text
SDL 事件
  → app::InputEvent
  → 当前 InputPort
  → 类型化 Intent 队列
  → LogicController::updateFixed
  → SceneCommand 队列
  → Window 命令屏障
  → RenderSnapshot
  → RenderView::prepare
  → RenderView::render
  → flush
```

`Application` 负责采集和调度时间；`Window` 负责活动场景编排、输入焦点和命令屏障；领域逻辑负责状态变更；渲染视图负责快照解释和表现缓存。

### 3.2 输入端口

输入层定义多态入口，不直接暴露场景类：

```cpp
class InputPort {
public:
    virtual ~InputPort() = default;
    virtual void accept(const app::InputEvent &event) = 0;
};
```

`accept()` 允许执行键位映射、文本规整、重复过滤和意图入队；禁止调用 `gWindow`、`gSaveData`、`gBag`、`Renderer`、事件 VM、战斗执行器和删除接口。意图为独立的多态值对象，例如：

- `MapMoveIntent`、`MapInteractIntent`、`OpenMainMenuIntent`；
- `BattleMoveCursorIntent`、`BattleConfirmTargetIntent`、`BattleChooseActionIntent`；
- `MenuMoveIntent`、`MenuConfirmIntent`、`MenuCancelIntent`；
- `TitleEditNameIntent`、`TitleConfirmIntent`、`TitleLoadSlotIntent`。

输入焦点由 `InputFocusChain` 管理。弹窗、战场和地图通过注册/撤销 `InputPort` 参与焦点链，公共路由器只调用当前端口的虚函数，不依据场景枚举或 `dynamic_cast` 选择行为。

### 3.3 逻辑控制器

逻辑控制器拥有意图队列和领域状态的写权限：

```cpp
class LogicController {
public:
    virtual ~LogicController() = default;
    virtual void updateFixed(const LogicTick &tick,
                             SceneCommandSink &commands) = 0;
};
```

逻辑控制器可以调用 `world`、`battle`、`event` 和纯资源查询接口，但不能包含 SDL 或 `Renderer` 头文件，也不能保存 `Texture*`。地图控制器保存位置、方向、动画帧、事件结果和 `SpriteId`；战斗控制器保存战斗会话、动作队列和工作背包；菜单控制器保存选择索引、文本编辑状态和候选结果。

### 3.4 场景命令

逻辑层不直接操作窗口和音频，而是提交可执行命令：

```cpp
class SceneCommand {
public:
    virtual ~SceneCommand() = default;
    virtual void execute(SceneCommandContext &context) = 0;
};
```

命令对象自行实现 `execute()`，避免 `Window` 中央 `switch`。典型命令包括 `OpenPopupCommand`、`SwitchMapCommand`、`StartBattleCommand`、`PlayMusicCommand`、`SaveGameCommand`、`LoadGameCommand`、`ShowMessageCommand`、`QuitCommand` 和 `BattleFinishedCommand`。

命令执行只发生在逻辑更新完成后的屏障。命令失败返回明确结果；需要改变持久状态的命令先构造候选对象，全部验证成功后一次提交，失败时保留原状态。命令执行期间产生的新命令进入下一轮屏障，避免递归修改当前遍历。

### 3.5 渲染快照与视图

渲染入口只接受值语义快照：

```cpp
class RenderView {
public:
    virtual ~RenderView() = default;
    virtual bool prepare(const RenderSnapshot &snapshot,
                         RenderContext &context) = 0;
    virtual void render(const RenderSnapshot &snapshot,
                        RenderContext &context) const = 0;
};
```

快照不得包含 `Renderer*`、`Texture*`、世界对象指针、命令回调或可变容器别名。地图快照至少包含位置、方向、相机目标、地形 revision、角色 `SpriteId`、事件帧和小地图数据；战斗快照至少包含参与者值副本、可选格、当前阶段、行动预览和效果值；菜单快照包含文本、选择索引、复选状态和尺寸。

纹理解析、字体图集、target texture 和绘制缓存属于 `RenderView`。`prepare()` 位于表现阶段，只能修改视图私有的 `RenderCache`，不能修改快照或任何逻辑对象。缓存构建使用临时对象：构建、锁定、解码或上传全部成功后替换旧缓存并提交 revision；任一步骤失败都保留旧缓存。`render()` 只读取已准备好的缓存和快照，不得写入逻辑 dirty 标记、命令队列、节点删除状态或 `Window::processingStage_`。

## 4. 多态输入消费方式

### 4.1 地图

`MapInputPort` 将方向键转换为 `MapMoveIntent`，确认键转换为 `MapInteractIntent`，取消键转换为 `OpenMainMenuIntent`。`MapLogicController` 调用移动解析器完成阻挡、位置提交、事件触发和转场命令；移动解析器只返回值类型结果，不调用窗口或音频。地图视图依据 `MapRenderSnapshot` 解析 `SpriteId`，不在移动函数中加载纹理。

### 4.2 战场

`WarfieldInputPort` 内部持有多态输入模式，而不是在 `Warfield::handleKeyInput()` 中检查 `stage_`：

- `MoveSelectingInput` 处理光标和移动目标；
- `AttackSelectingInput` 处理攻击目标；
- `ActionMenuInput` 处理技能、道具、投掷和休息；
- `PopupInput` 处理战斗内确认和取消。

各模式只生成 `BattleIntent`。`WarfieldLogicController` 让玩家和 AI 进入同一动作解析路径，并通过 `BattleEngine` 的工作副本与事务背包执行。`ItemView` 只提供选择结果，禁止直接调用无参数 `useItem()` 或访问生产 `gBag`。

战斗逻辑产生 `BattleEvent`，由场景适配器转换为 `SceneCommand` 或表现事件，例如音效、消息、效果动画和战斗结束；`battle` 命名空间不依赖 `scene`。

### 4.3 菜单与标题

`MenuInputPort` 和 `TitleInputPort` 分别生成选择与文本意图。选择索引、姓名草稿和确认状态由逻辑控制器维护；确认后才构造新游戏、装备、商店、保存、加载或设置命令。取消、资源缺失、存档失败和命令拒绝都只生成失败结果，不污染候选外的世界状态。

## 5. 迁移阶段

### 阶段 1：契约与适配器

新增输入端口、意图、逻辑控制器、命令和快照的最小接口，并为队列顺序、命令屏障和快照不可变性添加失败测试。旧节点输入方法只保留调用适配器，不得继续修改业务字段。

完成条件：应用仍能启动；现有 43 项测试保持通过；新增测试能证明输入阶段不改世界快照。

### 阶段 2：地图与事件

迁移 `src/scene/map_navigation.cc`、`src/scene/submap.cc`、`src/scene/globalmap.cc` 的移动、事件和转场逻辑。地图控制器只使用值类型和 `SceneCommand`；地图渲染改读快照，纹理加载移入视图资源服务。

完成条件：阻挡、移动事件、出入口、子地图切换、船只状态和原版动画频率回归通过；相同输入序列在不同渲染帧率下产生相同地图快照。

### 阶段 3：菜单、标题与道具

迁移 `src/scene/menu.cc`、`src/scene/window_menu.cc`、`src/scene/title.cc`、`src/scene/itemview.cc`。所有确认回调改为意图或命令；新游戏属性、姓名和商店交易采用候选对象并在成功后提交。

完成条件：取消、关闭、资源缺失、存档失败和购买失败均保持原状态；战斗期间不存在 `ItemView → gBag` 访问。

### 阶段 4：战场

迁移 `src/scene/warfield_input.cc`、`src/scene/warfield_ui.cc`、`src/scene/warfield_actions.cc`、`src/scene/warfield_ai*.cc`、`src/scene/warfield_turns.cc` 和 `src/scene/warfield_results.cc`。输入模式、动作执行、AI、回合推进和结算都通过控制器和 `BattleEngine` 边界；表现请求使用 `BattleEvent`。

完成条件：玩家与 AI 共用动作类型；事务背包、战斗回滚、随机调用顺序和战后提交回归通过；战斗逻辑文件不包含 SDL、`Renderer`、`gWindow` 或生产 `gBag`。

### 阶段 5：渲染纯度与缓存

迁移 `src/scene/map.cc`、`src/scene/globalmap.cc`、`src/scene/submap.cc`、`src/scene/warfield_render.cc`、`src/scene/nodewithcache.cc`、`src/scene/messagebox.cc` 和 `src/scene/title.cc`。所有视图改读快照；脏状态和 cache revision 归表现层；`prepare()` 构建失败保留旧缓存。

完成条件：连续渲染任意次数不改变逻辑状态哈希；未呈现帧不改变逻辑状态；渲染缓存失败时旧缓存仍可绘制。

### 阶段 6：删除过渡路径与门禁收紧

删除业务类中的 `handleKeyInput()`、输入阶段直接副作用、逻辑对象中的 GPU 指针和兼容转发层。重新配置 CMake，更新架构文档和扫描规则。

完成条件：源码扫描无旧输入入口、反向依赖和渲染写逻辑；Debug/Release 全量构建与 CTest 通过。

## 6. 测试与架构门禁

### 6.1 C++ 行为测试

- `tests/app/input_port_tests.cc`：输入映射、时间戳排序、重复事件和焦点链。
- `tests/scene/logic_command_tests.cc`：命令屏障、失败结果、延迟执行和下一轮命令。
- `tests/scene/map_logic_tests.cc`：移动阻挡、事件触发、转场结果和状态回滚。
- `tests/scene/menu_logic_tests.cc`：选择、取消、文本草稿、商店和存档失败。
- `tests/scene/warfield_input_tests.cc`：各多态输入模式、玩家/AI 意图一致性和战斗道具事务。
- `tests/scene/render_snapshot_tests.cc`：快照值语义、连续渲染不改逻辑状态、缓存失败保留旧版本。

### 6.2 Python 架构扫描

新增或扩展以下门禁：

- `tests/architecture/input_logic_boundary_tests.py`：输入实现禁止访问世界、窗口、音频和渲染对象。
- `tests/architecture/logic_render_boundary_tests.py`：逻辑头文件禁止 SDL/Renderer，逻辑实现禁止 `Texture*`。
- `tests/architecture/scene_command_boundary_tests.py`：场景副作用只能由 `SceneCommand::execute()` 发起。
- 扩展 `tests/architecture/render_purity_tests.py`：检查快照入口、状态哈希行为和缓存 revision 提交。
- 扩展 `tests/architecture/warfield_battle_boundary_tests.py`：扫描所有战斗相关文件及 `ItemView`，禁止生产 `gBag` 绕行。

### 6.3 验证命令

每个公共阶段完成后执行受影响测试；阶段 1、4、5、6 以及最终交付执行：

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build cmake-build-debug --config Debug
ctest --test-dir cmake-build-debug -C Debug --output-on-failure
cmake --build cmake-build-release --config Release
ctest --test-dir cmake-build-release -C Release --output-on-failure
git diff --check
```

新增 `*.cc` 或 `*.hh` 后，先重新运行对应的 CMake 配置，再构建。

## 7. 兼容性、错误处理与性能

- 输入意图必须保留原始 `timestamp` 和稳定序号；逻辑消费顺序不得受渲染帧率影响。
- 地图 BIOS tick 等效频率、战斗随机源调用顺序和事件 VM 等待/恢复顺序保持不变。
- 资源、存档、战斗和商店更新均先构造候选对象，成功后一次提交；失败路径保留旧对象和旧快照。
- 渲染缓存不得阻塞逻辑 tick；缓存构建失败时继续绘制上一个成功版本，并记录可诊断的失败状态。
- 快照只复制跨帧所需值，禁止整份存档和静态资源表重复复制；纹理按 `SpriteId` 懒解析，但解析属于表现层。
- `flush()` 的限帧、SDL present 和渲染器计时只属于平台/表现层，不得影响逻辑状态。

## 8. 最终完成标准

同时满足以下条件才视为重构完成：

1. 输入、逻辑、渲染依赖方向通过静态门禁，源码中不存在业务输入回调直接修改世界状态的路径。
2. 地图、菜单、标题和战场均通过类型化意图、逻辑控制器和场景命令工作；不同输入消费模式由多态对象承载。
3. 渲染只消费不可变快照，连续或跳过渲染帧不改变逻辑状态；表现缓存失败可回滚。
4. 战斗道具、存档、商店和新游戏初始化没有全局状态绕行，失败路径不污染既有状态。
5. 原版资源、存档、事件和战场格式保持兼容，随机调用顺序和固定逻辑频率保持兼容。
6. Debug 与 Release 构建、全量 CTest、架构扫描和 `git diff --check` 均通过，且无本轮新增警告未处理。
