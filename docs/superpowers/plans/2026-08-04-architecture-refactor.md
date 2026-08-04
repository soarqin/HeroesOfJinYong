# 全仓架构重构执行计划

> 状态：已完成（2026-08-04）。阶段 1—9 的代码迁移、物理拆分、事务边界和验收门禁均已实施并通过最终验收。
> 阶段 7 的明确边界是 `BattleEngine` session/transaction adapter；Warfield 仍在场景层执行
> 已确认的原版动作算法。当前 `BattleEngine::replay()` 是观察式快照链校验，校验动作、前后快照、
> 背包槽序和随机调用完整性，但不会再次执行完整的伤害或 AI 算法；这项边界不被描述为
> 「确定性动作重执行」。

## 目标与不可变边界

- 解决冷热数据强制物理分离造成的复杂度，同时保持 DOS 存档、IDX/GRP、事件和战斗数据格式不变。
- 将大文件按职责拆分，首批目标是 `src/scene/window.cc`、`src/scene/mapwithevent.cc`、`src/scene/warfield.cc`。
- 将输入采集、固定时间步模拟、事件推进和渲染提交分开；保留旧地图/战斗逻辑的 15 Hz 行为契约。
- 最终运行逻辑只依赖 `content`、`world`、`event`、`battle`、`scene`、`app`。迁移期可将 wire-format 适配器暂存于新域目录；全部调用点、测试和 CMake 依赖迁移完成后，必须删除 `hojy::data`、`hojy::mem`、旧 include 路径、兼容转发层和 `src/data`、`src/mem` 目录，不保留旧 namespace 适配器。
- 不修改 `deps/`，不改变已确认的战斗算法、随机调用顺序、原版证据边界或用户已有无关修改。

## 阶段 1：基线、目标库和测试入口

**关键修改**

- 在顶层 `CMakeLists.txt` 统一启用 `CTest`，在 `tests/CMakeLists.txt` 提供按领域运行的测试入口。
- 将 `src/CMakeLists.txt` 的源码目标改为可迁移的 `hojy_content`、`hojy_world`、`hojy_event`、`hojy_battle`、`hojy_scene` 静态库（第一步只建立空/薄目标，现有实现仍可链接）。
- 新增 `tests/architecture/namespace_scan_tests.py`，扫描源码和测试中是否意外新增旧 namespace 引用；迁移期允许白名单文件，最终白名单必须为空。
- 新增 `tests/architecture/build_contract_tests.py`，检查各目标可以单独配置并链接最小测试。

**已执行的关键文件/函数修改（2026-08-04）**

- `CMakeLists.txt`：统一启用 `CTest`，仅在 `BUILD_TESTING=ON` 时加载测试目录。
- `src/CMakeLists.txt`：建立 `hojy_content`、`hojy_world`、`hojy_event`、
  `hojy_battle`、`hojy_scene`、`hojy_app` 目标，并明确领域间链接依赖。
- `tests/CMakeLists.txt`：按 content、world、event、battle、scene、app 和 architecture
  注册测试；新增源码布局、构建边界、渲染纯度、音频回调、应用/场景和 Warfield/BattleEngine 门禁。
- `tests/architecture/namespace_scan_tests.py`、`tools/architecture/namespace_scan.py`：
  旧 namespace 和 include 扫描的允许列表为空，默认 CTest 直接扫描最终 `src` 和 `tests`
  源码树。

**验收**：Debug/Release 均可配置；现有 CTest 全部通过；记录基线行数、目标依赖和 namespace 扫描结果。

## 阶段 2：二进制 I/O、序列化与事务边界

**关键修改**

- 新增 `src/content/binary_reader.hh/.cc`：提供 `readExact`、范围/对齐/整数溢出检查和有界字符串读取；所有失败返回 `Expected`/`bool + error`，不推进游标。
- 新增 `src/content/atomic_file.hh/.cc`：临时文件写入、刷新、原子替换和失败清理。
- 将 `src/world/serializable.*` 的裸指针读取封装到有界读取逻辑；为 `GrpData`、
  `WarfieldData`、`SaveData` 增加候选对象提交边界。
- `src/world/savedata.cc`：`load()` 使用候选 `SaveData`，仅在所有层、事件和背包同步成功后提交；
  `save()` 先生成完整字节缓冲，写入成功后再更新存档槽位。
- 新增损坏输入、短读、越界和保存失败回滚测试，覆盖 `tests/content/persistence_tests.cc`
  与 `tests/content/binary_io_tests.cc`。

**已执行的关键文件/函数修改（2026-08-04）**

- `src/content/binary_reader.*`：`readExact()`、`readValue()`、`readString()` 统一检查
  短读、范围、元素对齐、整数转换和分配失败；失败时不推进游标。
- `src/content/atomic_file.*`：完成临时文件写入、刷新、替换和失败清理。
- `src/content/grpdata.cc:readFile/loadData/saveData`：校验 IDX/GRP 长度、偏移单调性、
  记录范围、`uint64_t` 到 `size_t` 转换和分配上限；失败时保留原数据集。
- `src/content/warfielddata.cc:load`：校验记录数、层大小、短读和超大分配，全部成功后提交候选层。
- `src/content/event.cc:load`：事件脚本与对话资源使用同一候选对象读取，避免只更新其中一个资源。
- `src/content/factors.cc:load`：区分 FishEdit 与原版布局，并校验所选布局的最小末尾偏移。
- `src/content/loader.cc:loadData`：`Factors`、`Event`、`WarfieldData` 全部加载成功后一次替换全局内容。
- `src/world/savedata.cc:load/save`：存档加载和写入采用候选对象；失败路径不修改现有存档与背包状态。
- `tests/content/persistence_tests.cc`、`tests/content/binary_io_tests.cc`：覆盖各加载阶段失败、
  短读、错误偏移、非对齐数据、超大尺寸和保存失败回滚。

**验收**：失败加载不污染旧状态；保存失败不改变当前存档和 `gBag`；原格式样本字节级兼容。

## 阶段 3：`content`/`world` 逻辑视图与冷热分离审查

**关键修改**

- 将资源定义、常量和静态表迁移到 `hojy::content`；`GrpData`、事件脚本和战场资源通过
  `content::StaticBundle` 暴露只读逻辑视图，不复制整张静态表。
- 保留存档 wire struct；`src/world/world_state.*` 提供 `world::SaveSnapshot`、
  `world::WorldStateView` 和 `world::InventoryView`，只复制跨帧运行所需字段。
- `src/battle/battle_participant.*` 用显式战斗工作副本和提交结果连接
  `world::state::CharacterData` 与战斗逻辑，避免把整个存档对象传入战斗模块。
- `world::state::Bag` 保留 `syncFromSave()`、`syncToSave()` 作为 wire-format 边界；战斗使用
  独立值副本并在结算时一次提交，避免双向隐式同步。
- 为静态资源不可变、快照字段完整性和提交原子性添加单元测试。

**已执行的关键文件/函数修改（2026-08-04）**

- `src/content/static_bundle.hh`：以常量指针组合 `Factors`、`Event`、`WarfieldData`，
  只暴露只读访问器。
- `src/world/world_state.*`：实现世界位置、队伍和背包快照的比较、构造与候选应用；
  `InventoryView::replace()` 先验证 ID、数量和容量，再更新 wire struct。
- `src/battle/battle_participant.*`：维护 baseline、working 和 staged candidate；
  `commit()` 与 `discard()` 明确区分正常结算和中止回滚。
- `tests/content/static_bundle_tests.cc`、`tests/world/world_state_tests.cc`、
  `tests/battle/battle_participant_tests.cc`：覆盖只读视图、快照应用、临时参与者和提交/回滚。

**验收**：运行逻辑不依赖静态资源对象的可变字段；快照可独立构造/比较；冷热分离只在边界发生，不产生重复全量数据。

## 阶段 4：大文件拆分与依赖注入

**关键修改**

- `window.cc` 当前保留构造、场景流转、更新/渲染/提交和 popup 生命周期；已将 `processEvents`、文本输入控制移到 `window_input.cc`，菜单及 `showMainMenu`/`runTalk`/`runShop`/`popupMessageBox` 移到 `window_menu.cc`，音频请求移到 `window_audio.cc`。阶段 5 接口化时，只有在调用契约稳定后才重命名为 `input_router.cc`、`scene_commands.cc`、`audio_bridge.cc`。
- `mapwithevent.cc` 当前只保留编译单元外壳；事件代码按 `map_event_runtime.cc`、`map_navigation.cc`、`map_event_extended.cc`、`map_event_interaction.cc`、`map_event_character.cc`、`map_event_story.cc`、`map_event_shop.cc` 拆分。`map_event_ops.cc` 仅保留迁移残留外壳，阶段 6 再将这些函数接入 `event::Vm`/`EventHost`，不得提前创建同名抽象。
- `warfield.cc` 当前只保留外壳；构造/加载、渲染、输入、回合、UI、行动、结算分别归入 `warfield_load.cc`、`warfield_render.cc`、`warfield_input.cc`、`warfield_turns.cc`、`warfield_ui.cc`、`warfield_actions.cc`、`warfield_results.cc`；`autoAction` 与技能定位分别归入 `warfield_ai.cc`、`warfield_ai_skill.cc`。
- 本阶段仅做物理拆分和依赖闭包修正，不改变函数体、事件 opcode 参数消费、pending 子事件顺序或战斗随机调用顺序。每新增一个 `*.cc` 后立即重新运行 CMake 配置并编译。
- 依赖注入先定义边界和测试替身，再逐步替换 `gWindow`、`gResource`、`gRandom`；旧全局暂时只允许出现在 `app/bootstrap.cc` 及明确列出的 wire-format 适配器中。
- `delete this`、`delete parent_` 和遍历期间删除节点列入阶段 5 的生命周期改造；本阶段不通过机械移动改变销毁时序。

**验收**：`tests/architecture/source_layout_tests.py` 对 `src/scene/*.cc` 的 600 行门禁通过；核心逻辑可在无 SDL 窗口的测试中构造；每个拆分函数在 `rg` 中恰好保留一个定义。

## 阶段 5：现代主循环、输入、节点和渲染缓存

**关键修改**

- `src/main.cc` 改为 `app::Application::run()`：采集 SDL 事件并写入 `InputQueue{timestamp, device, action, value, text}`，再按 60 Hz fixed tick 调用 `WorldScheduler`，最后提交渲染帧和 deferred commands。
- `FixedTickAccumulator` 每 4 个 60 Hz tick 触发一次 15 Hz compatibility tick，保留 `pressedKeys_` 的 180 ms 初始延迟和 20 ms 重复节奏；输入顺序以时间戳稳定排序。
- `src/scene/window.*` 提供 `dispatchInput(const InputEvent&)`、`updateFixed(Duration)`、`render(RenderContext&)`、`flush()` 四个阶段；`render()` 不得写入地图、事件或战斗状态。
- `src/scene/map.*` 将 `frameUpdate()` 从 `render()` 移到 fixed tick；新增 `TerrainCache`/dirty 标记，未失效时复用 RLE 解码和 composite buffer。
- 增加输入顺序、tick 频率、render 不改变状态、dirty 缓存失效和场景命令延迟执行测试。

**已执行的关键文件/函数修改（2026-08-04）**

- `src/app/application.cc`：`Application::run()` 固定为「采集输入 → fixed update → compatibility update → deferred barrier → render → flush」，不再让渲染阶段提交场景命令。
- `src/scene/window.cc`：`Window::render()` 删除 `applyDeferredNodes/applyDeferredCommands`；节点和场景命令只在 `updateFixed/compatibilityUpdate/dispatchInput` 的阶段边界提交。
- `src/scene/node.cc`：`doUpdate/doRender/doHandleKeyInput/doTextInput` 改用异常安全的 dispatch depth 回滚；子节点遍历继续使用快照和 deferred delete。
- `src/scene/nodewithcache.*`：新增 `NodeWithCache::update/rebuildCache`；缓存只在 update、`forceUpdate()` 或布局阶段重建，`render()` 只读现有 texture。
- `src/scene/messagebox.*`：`layoutText/ensureYesNoMenu` 从 `makeCache()` 拆出，`MessageBox::update()` 负责布局和 Yes/No 子树创建；不再清空 `text_` 或在 render 首次创建菜单。
- `src/scene/title.*`：新增 `Title::update/ensureConfirmationMenu`，确认菜单从缓存绘制函数迁移到 update 阶段。
- `src/scene/title.*`：新增 `prepareNewGame()`，检查新游戏存档、主角记录和初始子地图后才进入姓名确认；
  固定长度姓名写入前再次校验目标指针并限制尾部文本长度，避免核心资源缺失时空指针写入。
- `src/scene/itemview.*`：新增 `normalizeSelection/update`，移除 `makeCache()` 对 `currTop_` 的修正，并修复重复 `show()` 累积物品的问题。
- `src/scene/endscreen.*`：stage 3 的 `frameTotal_` 在输入/状态转换时计算；render 不再触发 cache rebuild。
- `src/scene/warfield_results.cc`：独立 root `statusPanel_` 由 Warfield 明确 delete，不再投递给 Window 无法观察的 deferred delete 队列。
- `tests/architecture/render_purity_tests.py`：新增缓存重建、popup 子树、场景命令和 ItemView 纯度门禁。

**验收**：去除 `goto eventStart`；相同输入序列在不同渲染帧率下产生相同世界状态；无 dirty 时不重复构建地图帧。

## 阶段 6：可暂停 Event VM

**关键修改**

- `src/event/vm.*` 定义 `ProgramCounter`、有界 `EventMemory`、操作预算和 `VmResult {Running, Waiting, Completed, Faulted}`。
- `src/scene/event_vm.*` 只负责解释指令；`event_host.*` 实现对话、道具、战斗和转场等宿主命令，并通过 `EventCommandSink` 排队副作用。
- 将 `runExtendedEvent()` 的全局 64K RAM、裸偏移和无界字符串替换为 `EventMemory::read/write`，每条指令检查边界；等待输入/动画时保存 PC 和栈，不阻塞主循环。
- 为分支、暂停/恢复、预算耗尽、非法 opcode、字符串越界和副作用顺序增加测试。

**已执行的关键文件/函数修改（2026-08-04）**

- `src/event/event_memory.*`：建立 64K word、带有符号地址范围和有界 CString 的事务内存；越界、未终止字符串和短写均不更新已有状态。
- `src/event/vm.*`：新增 `Vm::step()` 宿主边界；纯内存 opcode 由 VM 执行，宿主 opcode 返回 `Waiting/Completed/Faulted`。
- `src/scene/mapwithevent.hh`：`MapWithEvent` 实现 `event::VmHost`，每个地图实例拥有独立 `event::Vm`，删除静态 `extendedRAMBlock_/extendedRAM_`。
- `src/scene/map_event_extended.cc`：`runExtendedEvent()` 改为构造 `event::Instruction` 并调用 `Vm::step()`；字段、子地图层、字符串、菜单和扩展节点命令均使用边界检查，移除 `reinterpret_cast`、`sprintf`、`strlen`、`strcat` 和无界内存访问。
- `src/scene/map_event_runtime.cc`：`cleanupEvents()` 清理 VM 内存、pending 子事件、移动/动画状态和扩展节点；等待型扩展节点通过 handler 恢复事件推进。
- `src/scene/map_event_character.cc`：扩展比较结果从 `event::Vm` 内存读取。
- `tests/event/event_vm_tests.cc`：覆盖有符号地址、CString 边界、操作预算、宿主等待/恢复和越界故障。

**验收**：事件解释器可独立跑完纯脚本；暂停期间主循环继续更新；错误脚本不会破坏世界状态。

## 阶段 7：`BattleEngine` 边界

**关键修改**

- 新增 `src/battle/engine.hh/.cc`：`begin(BattleSetup)`、`record(BattleAction, InventorySnapshot)`、
  `reconcile()`、`snapshot()`、`finish(bool)`、`abort()`；内部只使用 battle 纯逻辑类型和
  `BattleParticipant`。不引入未落地的 `step(Input/AI decisions)` API。
- 将 `warfield` 的加载、回合、行动、AI、结算分别接到 engine adapter；保留随机源调用顺序并记录
  `RandomCall`，把战后结算随机从最后一个动作的随机区间中分离。
- `Warfield` 仅负责 scene 生命周期和渲染适配，不再持有完整存档/资源全局；结束时以事务结果提交经验、物品和任务状态。
- 增加快照链回放校验、胜负结算回滚、AI/玩家输入隔离和快照一致性测试；完整动作 executor
  不在本轮边界内。

**已执行的关键文件/函数修改（2026-08-04）**

- `src/scene/warfield.hh`：引入 `battle/engine.hh`，新增
  `std::vector<std::unique_ptr<battle::BattleParticipant>> battleParticipants_`
  与 `battle::BattleEngine battleEngine_`；`putChars()` 改为返回 `bool`，并声明
  `syncBattleParticipantsToWorking()`、`syncBattleParticipantsFromWorking()`、
  `discardBattleSession()`。
- `src/scene/warfield_load.cc:cleanup`：清理角色数组前先结束或回滚 BattleEngine，
  将 participant 工作副本恢复到 `CharInfo::info` 后再销毁 participant，避免悬空引用。
- `src/scene/warfield_load.cc:putChars`：使用局部 `nextChars` 和占用格候选表，完成
  角色位置、重复格、装备加成、敌方入场属性及 AI 快照后一次提交；随后按角色槽位建立
  participant，并在玩家/敌方均存在时调用 `BattleEngine::begin()`。候选构造或引擎启动失败
  会清空候选并返回失败，不继续进入淡入战场流程；`validateUniqueWarfieldCharacterIds()`
  拒绝重复持久角色 ID，避免同一存档角色被加入两次并在结算时重复写回。
- `src/battle/engine.cc:validInventoryTransition()`：按 `InventorySnapshot` 向量的原始顺序
  比较背包，而不是折叠成 map；PartyBag 消耗按槽序执行单项扣减，NPC carry 消耗校验
  `slot`、物品 ID、数量和尾槽压缩结果。
- `src/battle/engine.cc:validParticipantTransition()`：动作只允许 actor 以及显式目标参与者
  发生状态变化；`NoOpAction` 要求全体参与者字节级不变。记录校验失败时恢复到上一条已接受
  快照，避免 `Faulted` 状态暴露半提交的 participant 或 inventory。
- `src/battle/action.hh`：加入 `NoOpAction` 和 `ItemAction::slot`；无效目标记录显式无状态动作，
  非法动作仍拒绝且不写入 action log。
- `src/battle/engine.hh/.cc:BattleReplay`：增加 `initialIntegrity`、`committed` 和
  `settlementRandomBegin`；final integrity 绑定最终快照、提交状态和结算随机边界，回放拒绝
  未提交结果、缺失 final snapshot、随机 raw value 篡改和边界错位。
- `src/scene/mapwithevent.hh/.cc`：为拆分后的多继承场景提供显式 out-of-line 析构函数，固定
  `MapWithEvent` 的 LTO key function，避免 Release 链接产生重复析构 thunk。
- `src/scene/warfield_turns.cc:checkWarEnd`：在存活计数回退逻辑前同步角色工作副本，调用
  `battleEngine_.reconcile()`；引擎报告 Finished 时以 snapshot 的 `won` 进入结算。
- `src/scene/warfield_results.cc:endWar`：Finished session 先在 staged participant 和
  `battleBag_` 上执行原有选择性经验、HP、MP、技能和制药结算；随后以最终 participant/inventory
  调用 `reconcile()`，只有 `finish(true)` 返回 `committed` 才写回 `gSaveData` 并交换全局背包。
  结算随机从 `settlementRandomBegin` 起记录，不归入最后一个 action；非正常状态调用
  `abort()` 并丢弃候选。
- `src/world/bag.*`、`src/world/action.*`：`Bag` 明确支持值复制和 `swap()`；
  `useItem()`、`tryUseBagItem()` 新增显式 `Bag` 参数重载，旧公共调用仍转发到 `gBag`。
- `src/scene/warfield_{actions,ai,ai_skill,results}.cc`：玩家投掷、AI 选药/用药和制作物品
  只访问 `battleBag_` 工作副本；`endWar()` 全部角色和物品结算完成后调用
  `commitBattleBag()` 一次提交，`cleanup()` 中止路径直接丢弃副本。
- `src/scene/window.cc:enterWar`：处理 `putChars()` 失败返回值，失败时恢复原地图且不执行
  战场淡入。
- `tests/architecture/warfield_battle_boundary_tests.py`：门禁要求 Warfield 持有事务性
  battle session、`putChars()` 返回失败状态、`checkWarEnd()` 使用 reconcile、`endWar()`
  使用 finish，并禁止行动/AI/结算文件直接访问生产 `gBag`；`tests/battle/*` 覆盖
  participant、engine 和 Bag 工作副本的提交/回滚基础契约。

**当前边界说明**：Warfield 的既有 AOE、治疗、投掷、道具、休息和回合末结算算法仍保留在
  `warfield_actions.cc`、`warfield_ai*.cc`、`warfield_turns.cc` 和 `warfield_results.cc`；本次
  adapter 只接管 session 生命周期、动作观察记录、快照校验和胜负/提交边界，未把动作重新
  执行一遍，以避免重复写状态和改变原版随机顺序。

**验收**：battle 逻辑可在无 SDL 的测试中完整运行；渲染帧率不影响回合结果；已有战斗回归测试全绿。

## 阶段 8：音频实时边界

**关键修改**

- `src/audio/mixer.*` callback 只做锁内 ring-buffer 读和混音；禁止文件 I/O、解码、内存分配、设备重初始化和 channel 容器修改。
- 主线程完成 WAV/音效加载、格式转换和 channel 生命周期；设备切换前停止 callback、清理旧格式缓存，再原子替换共享状态。
- SDL 音频长度转换前检查 `int` 范围，处理 `SDL_ConvertAudio`、分配和设备初始化失败，并添加失败回滚测试。

**已执行的关键文件/函数修改（2026-08-04）**

- `src/audio/mixer.*`：新增主线程 `Mixer::service()`；淡入淡出、文件加载、声道替换和结束清理均移出 callback。
- `src/audio/mixer.cc`：`Mixer::callback()` 只在统一 `playMutex_` 下读取已存在声道、混音并设置 `ended` 标志；不再调用 `SDL_GetTicks`、`new/delete`、`load/start/reset`、文件路径解析或容器扩容。
- `src/scene/window.cc`：fixed update 开始时调用 `audio::gMixer.service()`，为实时 callback 提供明确的主线程维护边界。
- `src/audio/channel.cc`：拒绝非空长度但空数据指针，避免构造时无效拷贝。
- `tests/architecture/audio_callback_purity_tests.py`：静态禁止 callback 内生命周期、时钟和 I/O 操作，并要求维护逻辑存在于 `service()`。

**验收**：音频 callback 静态审查无阻塞/分配调用；设备重启后旧声道不泄漏；Debug/Release 音频测试全绿。

## 阶段 9：删除旧 namespace 与最终验收

**关键修改**

- 将全部运行逻辑、wire-format 适配器 include、符号和测试迁移到 `content/world/event/battle/scene/app`；不得新增对旧目录的依赖。
- 只有 namespace 扫描、全量构建和 wire-format 回归均证明无依赖后，才删除 `namespace hojy::data`、`namespace hojy::mem`、旧 include 路径、兼容转发头和 `src/data`、`src/mem` 目录；CMake 同步停止收集已删除目录。
- 将 namespace 扫描测试的迁移白名单设为空，并把扫描加入默认 CTest/CI。
- 删除旧 API 文档和兼容说明；更新架构文档，记录每个关键文件/函数的最终归属和删除原因。

**已执行的关键文件/函数修改（2026-08-04）**

- `src/data/*` 已迁移到 `src/content/*`，最终 namespace 为 `hojy::content`；
  `src/mem/*` 已迁移到 `src/world/*`，wire struct 保留在 `hojy::world::state`。
- `tests/data/*`、`tests/mem/*` 已分别迁移到 `tests/content/*`、`tests/world/*`
  或相应 battle 测试目录；CMake 不再收集旧目录。
- `src/data`、`src/mem`、`tests/data`、`tests/mem` 已删除；不存在旧 include 转发头、
  namespace alias 或兼容适配层。
- `tests/architecture/namespace_scan_tests.py` 的允许列表为空；扫描仅匹配 C++ namespace、
  限定名与 include，不把 `src/config.toml` 中的运行时资源目录 `data/font/chinese.otf`
  误判为源码依赖。历史文档中的旧路径引用也已改为 `content/world` 新归属；计划中的
  「删除旧 namespace」文字仅作为迁移决策记录，不是代码依赖。

**最终验收**

```powershell
$env:PATH='C:\msys64\ucrt64\bin;' + $env:PATH
cmake --build cmake-build-refactor-debug3 --config Debug
ctest --test-dir cmake-build-refactor-debug3 -C Debug --output-on-failure
cmake --build cmake-build-refactor-release3 --config Release
ctest --test-dir cmake-build-refactor-release3 -C Release --output-on-failure
git diff --check
rg -n 'namespace hojy::(data|mem)|hojy::(data|mem)::|"(data|mem)/' src tests CMakeLists.txt
```

扫描必须无匹配；存档/资源样本和战斗回放回归必须保持通过。

**实际验证记录（2026-08-04）**

- Debug/Release 使用 `cmake-build-refactor-debug3`、`cmake-build-refactor-release3`，两套配置
  均构建成功并注册 43 个 CTest；Debug 和 Release 均为 `43/43` 通过（退出码 0）。
- `git diff --check` 返回 0；仅保留工作区已有的 LF/CRLF 转换提示。
- 旧 namespace/include 在 `src`、`tests` 和 CMake 源码扫描中均无匹配；`src/data`、`src/mem`、
  `tests/data`、`tests/mem` 目录不存在。
- Release 的 `libADLMIDI`、`fmt`、`zita-resampler` 和既有菜单宽字符串警告属于既有依赖或
  场景代码；本轮新增的 `MapWithEvent` LTO 重复析构链接错误已通过显式析构函数消除。
