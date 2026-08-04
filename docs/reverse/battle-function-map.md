# 原版战斗函数映射

本页把 IDA Pro 9.4 对 `Z.DAT` 的函数观察与 C++ 实现对应起来。地址仅适用于基线清单中的 `Z.DAT` 哈希。

| 地址 | IDA 函数 | 观察结果 | C++ 对应 |
| --- | --- | --- | --- |
| `0x31EB9` | `sub_31EB9` | 初始化 26 个角色槽，写入阵营、坐标和战斗数据，循环驱动绘制、输入与行动 | `scene::Warfield::nextAction`、`autoAction` |
| `0x3271E` | `sub_3271E` | 每回合按角色槽顺序行动；移动步数为 `max(0, speed / 15 - hurt / 40)`，完整角色循环结束后调用回合结算 | `battle::buildRoundQueue`、`battle::calculateMovementSteps`、`scene::Warfield::nextAction` |
| `0x33599` | `sub_33599` | AI 依次检查低体力、生命与受伤、解毒、内力、医疗支援和解毒支援；各比例阈值使用 `sub_3D612(10)` 短路判断 | `battle::ai_policy`、`Warfield::autoAction` |
| `0x33C4D` | `sub_33C4D` | 生命恢复分支依次尝试自身医疗、按来源槽位返回首个正生命物品和请求队友医疗；合格提供者使当前槽保存行动代码 8 | `battle::chooseFirstResourceItem`、`battle::chooseMedicProvider`、`Warfield::autoAction` |
| `0x33E93` | `sub_33E93` | 解毒分支依次尝试自身解毒、按来源槽位返回首个负中毒物品和请求队友解毒；合格提供者使当前槽保存行动代码 9 | `battle::chooseFirstResourceItem`、`battle::chooseDepoisonProvider`、`Warfield::autoAction` |
| `0x340D9` | `sub_340D9` | 内力不足时按来源槽位返回首个正内力物品 | `battle::chooseFirstResourceItem`、`Warfield::autoAction` |
| `0x341F6` | `sub_341F6` | 按角色槽顺序选择首个可医疗队友；依次检查行动代码 8、生命低于 20、受伤高于 40，以及 `1/2`、`1/3`、`1/4`、`1/5` 生命门槛 | `battle::chooseMedicSupportTarget` |
| `0x343DA` | `sub_343DA` | 按角色槽顺序选择首个可解毒队友；依次检查行动代码 9，以及中毒高于 10、20、30、40 的门槛 | `battle::chooseDepoisonSupportTarget` |
| `0x34550` | `sub_34550` | 资源分支未选中行动后，先按团队强弱选择支援，再按上毒、暗器、体力和最低内力门槛选择行动 | `battle::chooseAiFollowupAction`、`Warfield::autoAction` |
| `0x34AEC` | `sub_34AEC` | 在恰好消耗剩余步数的可达格中最大化与敌方槽位的距离总和，移动后休息或继续后续流程 | `battle::chooseRetreatPosition`、`Warfield::doRest` |
| `0x34C47` | `sub_34C47` | 统计正武功 ID 数量后均匀选择槽位，按目标策略检查施术范围和移动 | `battle::chooseOriginalSkillSlot`、`battle::chooseAiTarget` |
| `0x3505B` | `sub_3505B` | integrity、potential 和随机门槛决定攻击最高、最低、能力或最近目标；同值保留首槽 | `battle::chooseAiTarget` |
| `0x3598C` | `sub_3598C` | 处理物品与暗器造成的生命、受伤和中毒变化 | `battle::applyThrow`；暗器状态写入已按 `BATTLE-ACT-THROW` 同步 |
| `0x36133` | `sub_36133` | NPC 物品耗尽后逐个移动 16 位物品与数量槽，并清空末槽 | `world::state::compactCarryItemSlots` |
| `0x361AC` | `sub_361AC` | 行动代码 8 的请求者可先接近医疗提供者，再直接进入普通武功选择；代码 8 保留 | `battle::chooseApproachPosition`、`battle::actionCodeForSkill` |
| `0x36209` | `sub_36209` | 行动代码 9 复用请求者接近与普通武功选择流程；代码 9 保留 | `battle::chooseApproachPosition`、`battle::actionCodeForSkill` |
| `0x36210` | `sub_36210` | 医疗距离为 `medic / 15 + 1`；距离不足时沿可达路径移动，无法进入范围时按攻击值与己方平均值选择休息或武功分支 | `battle::chooseSupportPosition`、`chooseUnreachableSupportFallback`、`Warfield::autoAction` |
| `0x363AC` | `sub_363AC` | 解毒距离为 `depoison / 15 + 1`；移动、施术和失败回退与医疗分支一致 | `battle::chooseSupportPosition`、`chooseUnreachableSupportFallback`、`Warfield::autoAction` |
| `0x3650E` | `sub_3650E` | 直线、区域和普通模式分别搜索施术格或按「上、右、左、下」逐步逼近目标 | `Warfield::autoAction` 内部局部规划、`battle::chooseSupportPosition` |
| `0x37734` | `sub_37734` | 统计可用武功，读取熟练度和范围，验证目标与移动路径；内力消耗为 `reqMp * ((level + 1) / 2)` | `Warfield::tryUseSkill`、`battle::calcRealSkillLevel`、`battle::getSelectableArea` |
| `0x38999` | `sub_38999` | 按四个方向逐格枚举直线攻击目标，记录命中格和距离 | `Warfield::startActAction` 的直线与十字分支 |
| `0x39188` | `sub_39188` | 计算武功伤害、受伤增长和附加中毒；主要波动使用 `random(20)`，负值修正使用 `random(4)`，武功附毒采用 1 基等级 | `battle::applyDamage` |
| `0x3B6BE` | `sub_3B6BE` | 升级时按潜能和随机值增加属性，生命、内力、体力恢复到上限 | `world::state::actLevelup`，仍需补充逐字段回归样本 |
| `0x3C563` | `sub_3C563` | 回合收束时扣除 `hurt / 20` 与 `poisoned / 10`，生命最低保留 1 | `battle::applyRoundEndDamage` |
| `0x3D612` | `sub_3D612` | 上界在 2..30000 时返回随机值模上界；其他上界返回 0 且不消费随机值 | `battle::GameRandom::next(int)`、`battle::SequenceRandom::next(int)` 复刻边界，`battle::originalRandom` 作为公式包装层 |

## 当前同步边界

- 已迁移到 `hojy_battle` 的算法均通过固定随机序列测试，测试覆盖随机调用顺序和状态写入结果。
- `Warfield` 仍负责动画、菜单、弹窗和资源生命周期；这些表现层逻辑没有进入纯战斗库。
- 原版 AI 的资源门槛、随机消费顺序、行动代码 `8/9`、队友支援目标、请求者接近移动和不可达回退判定已提取到 `battle::ai_policy` 与 `Warfield::autoAction`。
- `world::state::tryUseNpcItem` 与 `world::state::tryUseBagItem` 已拆出 `battle::chooseFirstResourceItem`；按来源顺序返回首个属性方向合格的治疗、内力、体力或解毒物品，不按数值差值排序，也不额外消费随机值。
- 场景适配的 `CharInfo::actionCode` 中，`8/9` 表示请求医疗或解毒，接近提供者后仍保留，供同回合后续队友读取；纯 AI 门面使用的 `AiRequest` 会在适配层映射到这两个值。行动代码 `4/5` 表示直接支援动作，不可达时进入休息或武功回退并继续保留。`battle::actionCodeForSkill` 只在这些续行动路径保留当前标记。
- 支援回退比较使用 `putChars()` 保存的基础攻击值；装备攻击只在伤害计算中叠加。
- 暗器、武功和行动 11 的原版优先级已由 `BATTLE-AI-FOLLOWUP`、`BATTLE-AI-RANDOM-SKILL`、`BATTLE-AI-RETREAT` 和 `BATTLE-AI-TARGET-STRATEGY` 确认；C++ 场景已完成对应接入。
- `Warfield::autoAction` 的目标评分使用 `battle::terrainPathDistance`，只读取地形阻挡；实际移动和施术使用占位感知的 `battle::shortestPathDistance`/可达格，并允许进入目标敌人的占位格。离场坐标在场景适配层标记为 inactive。
- `chooseOriginalSkillSlot` 按正武功 ID 的随机序号回扫原始槽位，空槽不会被误选；场景适配仍可传入紧凑列表。

## 角色战斗字段

角色记录包含 91 个 `int16` 字段，单条记录为 182 字节。战斗公式已确认以下偏移：

| IDA 地址 | C++ 字段 |
| --- | --- |
| `word_9016E` | `hp` |
| `word_90170` | `maxHp` |
| `word_90172` | `hurt` |
| `word_90174` | `poisoned` |
| `word_90176` | `stamina` |
| `word_9019E` | `mp` |
| `word_901A0` | `maxMp` |
| `word_901A2` | `attack` |
| `word_901A4` | `speed` |
| `word_901A6` | `defence` |
| `word_901A8` | `medic` |
| `word_901AA` | `poison` |
| `word_901AC` | `depoison` |
| `word_901AE` | `antipoison` |
| `word_901B8` | `throwing` |
