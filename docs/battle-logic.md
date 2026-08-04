# 战斗逻辑参考

本页是战斗系统的实现参考，按「基础设施 → 流程 → 数值 → 范围 → AI → 战后结算」的顺序记录全部规则，便于修改与排查。

相关文档的分工：

- 本页：规则本身、对应代码位置，以及修改时的连带影响。
- `docs/battle-math.md`：只保留数学规则与 AI 决策，不包含代码位置和逆向工程过程。
- `docs/reverse/battle-evidence.md`：原版二进制的加载方式、数据结构偏移、函数锚点与逐指令结论。
- `docs/reverse/battle-function-map.md`：原版函数地址与当前 C++ 实现的职责映射。
- `docs/reverse/battle-behavior-matrix.md`：每项行为的证据状态、已修复差异、有意偏离与未复刻项。

规则后面括号内的十六进制地址，是原版 `Z.DAT` 加载映像中的位置，仅对 `battle-evidence.md` 登记的 SHA-256 有效。

## 1. 代码位置索引

| 层次 | 文件 | 职责 |
| --- | --- | --- |
| 纯数值规则 | `src/battle/formulas.hh` / `.cc` | 伤害、中毒、医疗、解毒、暗器、休息、回合末结算、武功等级与内力消耗、升级成长 |
| AI 规则 | `src/battle/ai_policy.*`、`ai_strategy.*`、`ai.*` | 资源决策、后续行动、目标与武功槽选择；`ai.*` 仅提供兼容门面 |
| 移动与范围 | `src/battle/movement.*`、`attack_area.*` | 地形距离、占位感知移动、施术站位和攻击格遍历 |
| 资源槽序 | `src/battle/resource_items.*`、`src/mem/item_slots.*` | NPC 携带槽、玩家背包保存槽和耗尽压缩 |
| 回合顺序 | `src/battle/turn_order.hh` / `.cc` | 行动速度、移动步数、回合队列排序 |
| 随机源 | `src/battle/random.hh` / `.cc`、`game_random.hh` / `.cc` | 随机接口、固定序列实现、游戏随机源适配 |
| 角色状态变更 | `src/mem/action.cc` | 把纯规则套用到 `mem::CharacterData`，负责钳制与状态写入 |
| 战场适配 | `src/scene/warfield.cc` | 回合调度、范围枚举、目标枚举、AI 快照与执行、战后结算 |
| 常量 | `src/data/consts.hh` | 各属性上限、学习槽位数、经验上限 |

测试：`tests/battle/formula_tests.cc`、`combat_rules_tests.cc`、`ai_tests.cc`、`ai_policy_tests.cc`、`ai_strategy_tests.cc`、`movement_tests.cc`、`attack_area_tests.cc`、`resource_items_tests.cc`、`turn_order_tests.cc`，以及 `tests/battle/random_tests.cc`。每条断言的注释标注对应地址或控制流契约。

修改任一数值规则时的最小动作：改 `src/battle/formulas.cc`，同步 `formulas.hh` 的注释与地址，补或改 `formula_tests.cc` 的对应用例，然后在 `battle-behavior-matrix.md` 记录状态变化。

## 2. 基础设施

### 2.1 随机数

原版随机数是线性同余（`0x3F98D`）：

```text
seed = seed * 0x41C64E6D + 0x3039
rand() = (seed >> 16) & 0x7FFF
```

取随机范围的包装函数（`0x3D612`）有一条容易忽略的规则：

```text
rnd(n) = 0                 当 n <= 1 或 n > 30000，且不推进随机数
rnd(n) = rand() % n        其余情况，取值 0..n-1
```

「不推进随机数」的意思是：随机调用的次数随参数取值而变化。随机接口分为三层：

- `RandomSource::next(upperExclusive)` 复刻原版上界独占契约；上界不在 2..30000 时返回 0，且不消费随机值。
- `battle::originalRandom(random, bound)` 是数值公式使用的包装层，统一委托给上述原版契约。
- `RandomSource::next(minimum, maximum)` 是独立的闭区间契约，取值包含两端；`minimum > maximum` 时抛出异常，不套用原版无消费边界。

所有原版战斗公式必须通过 `battle::originalRandom` 取随机数，否则随机序列会与原版错位。

**未复刻**：本实现不复刻 LCG 本身，`util::gRandom` 使用 `mt19937_64`。随机调用的次数、顺序与取值范围与原版一致，但同一存档不会得到相同的随机序列。

`battle::SequenceRandom` 用固定序列驱动规则，`callCount()` 与 `calls()` 用于断言随机消耗次数，是回归测试的主要手段。

### 2.2 整数语义

原版在 16 位寄存器里运算这些值，只在表达式末尾用 `cwde` 扩展为 32 位，因此中间结果会在 16 位处回绕。`formulas.cc` 内的 `narrow()` 复现这一截断。除法一律采用 C 语言的向零取整。

角色数值本身都是 `int16_t`，写入前按 `src/data/consts.hh` 的上限做钳制。

### 2.3 关键数据字段

数值规则用到的字段（完整偏移表见 `battle-evidence.md`）：

| 类别 | 字段 |
| --- | --- |
| 角色状态 | `hp`、`maxHp`、`hurt`、`poisoned`、`stamina`、`mp`、`maxMp`、`level`、`exp` |
| 角色属性 | `attack`、`speed`、`defence`、`medic`、`poison`、`depoison`、`antipoison`、`fist`、`sword`、`blade`、`special`、`throwing`、`knowledge`、`integrity`、`poisonAmp`、`doubleAttack`、`potential` |
| 角色装备与武功 | `equip[2]`、`skillId[10]`、`skillLevel[10]`、`learningItem`、`expForItem`、`expForMakeItem`、`item[4]`、`itemCount[4]` |
| 武功 | `reqMp`、`addPoison`、`skillType`、`attackAreaType`、`damageType`、`damage[10]`、`selRange[10]`、`area[10]`、`addMp[10]`、`drainMp[10]` |
| 物品 | `addHp`、`addMp`、`addPoisoned`、`addAttack`、`addSpeed`、`addDefence`、`throwingEffectId`、`itemType` |

`skillLevel[i]` 是 0..999 的原始值，`skillLevel[i] / 100` 才是 0..9 的等级。要注意区分「原始等级」与「内力降级后的等级」，两者在不同规则中各有使用。

`scene::Warfield::CharInfo` 是战场内的角色副本，额外持有 `side`、`x`、`y`、`steps`、`initialSteps`、`exp`、`actionCode`。其中 `actionCode == 8/9` 是战斗副本内持久的医疗/解毒请求标记，不是仅用于界面的瞬时行动码；纯 AI 门面仍使用 `AiRequest`，由场景适配层映射为行动码。战斗期间只改副本，战斗结束才回写存档。

## 3. 战斗流程

### 3.1 回合循环（`0x3271E`）

```text
每回合：
  1. 重排行动顺序
  2. 为每个参与者重算移动步数
  3. 按顺序派发行动，逐个执行
  4. 回合末对所有参与者结算受伤与中毒
```

对应实现在 `Warfield::nextAction`：`charQueue_` 空时先做回合末结算（首回合除外），再自增回合数、重建队列、重算步数。队列从 `back()` 消费，等价于原版按正序遍历。

### 3.2 行动顺序（`0x32A51`）

排序键是 `speed + 武器 addSpeed + 防具 addSpeed`，按降序排列。算法为选择排序加即时交换：外层指针遍历每个槽位，内层与后续各项逐一比较，发现更快的立即交换。这个「即时交换」决定了同速者的相对顺序，不能换成 `std::stable_sort`。

`battle::sortActionOrder` 是这段逻辑的实现，`buildRoundQueue` 在排序后过滤掉已阵亡者。

### 3.3 移动步数（`0x328A7`）

```text
steps = max(0, (speed + 武器 addSpeed + 防具 addSpeed) / 15 - hurt / 40)
```

`battle::calculateMovementSteps` 实现该式。本实现中，`CharInfo::info.speed` 已由 `mem::addUpPropFromEquipToChar` 并入装备加成，因此只需传入 `speed` 与 `hurt` 即可。`initialSteps` 记录本回合发放的步数，供休息判定使用。

### 3.4 等待（`0x3AA17`）

把当前行动者与后续参与者依次交换，相当于移到队尾。原版主循环收到返回值 6 时重跑同一槽位，此时槽位上已换成别的角色。

本实现在 `playerMenu` 中把 `charQueue_` 里的当前角色移到队首（队列从尾部消费，队首即最后行动）。

### 3.5 回合末结算（`0x3C563`）

```text
对每个参与者：
  若 hurt > 0                         → 执行
  否则要求 poisoned > 0 且 stamina > 0 且未退场，才执行
执行内容：
  hp -= hurt / 20
  hp -= poisoned / 10
  stamina < 0 → 1
  hp < 1      → 1（当前修正版）
```

`battle::roundEndDrain(hurt, poisoned)` 只算扣血量，`mem::actRoundEndDrain` 负责守卫与写入。

**有意偏离**：原版在 `hurt > 0` 时跳过退场与生命检查，且只在 `hp < 0` 时改为 1；结果恰为 0 时仍保留 0，已阵亡且带受伤值的角色也可能被改回 1。本实现先排除已阵亡者，并统一执行 `hp < 1 → 1`。

### 3.6 胜负判定（`0x3B238`）

每次行动结束后运行：

```text
1. 生命 <= 0 且未退场者：清空所占格子，置退场标记
2. 统计双方是否还有在场者
3. 己方全灭 → 失败；敌方全灭 → 胜利；两者同时成立时结果为胜利
```

`Warfield::checkWarEnd` 先判敌方全灭，与原版「胜利覆盖失败」的净效果一致。占位格清理在 `Warfield::endTurn` 中完成。

## 4. 玩家行动菜单（`0x32E59`）

体力比较全部使用 `jle`（小于等于则跳转），阈值不含等号。

| 菜单项 | 开放条件 |
| --- | --- |
| 移动 | `stamina > 5` 且剩余步数 > 0 |
| 武功 | `stamina > 10` 且 `mp >= 已学武功中最小的 reqMp` |
| 用毒 | `stamina > 10` 且 `poison >= 20` |
| 解毒 | `stamina > 50` 且 `depoison >= 20` |
| 医疗 | `stamina > 50` 且 `medic >= 20` |
| 物品、等待、状态、休息、自动 | 始终开放 |

「武功」子菜单按 `mem::calcRealSkillLevel` 返回 -1 来过滤单条武功，判据与菜单门一致（`mp >= reqMp`）。

## 5. 数值公式

### 5.1 武功等级与内力消耗

```text
内力消耗（0x3850E）：cost(level) = reqMp * ((level + 1) / 2)          level 为 0 起
可用等级（0x39217）：从 skillLevel / 100 递减，取第一个满足 mp >= cost(level) 的等级
```

`level = 0` 的消耗为 0，但玩家首次打开武功菜单并选择武功时仍要求 `mp >= reqMp`。0 级只用于武功已经选定后的双击续击，以及 AI 请求者接近等强制续行动路径；这些路径不会因第一击耗尽内力而取消后续攻击。

`battle::skillMpCost` 与 `battle::resolveSkillLevel` 实现这两式。`resolveSkillLevel` 在 `reqMp > 0 且 mp < reqMp` 时返回 -1，用于菜单过滤；这一层是本实现的界面契约，原版靠菜单门实现同样效果。

内力在**每次攻击**扣除一次，不按受击目标逐个扣除。区域武功打中五个目标也只扣一份，由 `mem::postDamage` 保证。

### 5.2 实际攻击与实际防御

```text
实际攻击（0x39294）：
  (attack * 3 + skill.damage[level]) / 2
  + 武器 addAttack + 防具 addAttack
  + 兵器绑定加成
  + 己方知识项

实际防御（0x39331）：
  defence + 防具 addDefence + 武器 addDefence + 对方知识项
```

`attack` 与 `defence` 指**不含装备**的角色基础值。本实现中，`info.attack` 已并入装备加成，`mem::calcRealAttack` 先减去装备加成再按上式重算，`mem::calcRealDefense` 直接使用已含装备的 `defence`。

兵器绑定（`0x377DC`）：`data::gFactors.skillWeaponsBindings` 是 7 组三元组 `{装备 id, 武功 id, 加成}`，当 `equip[0]` 与武功 id 同时匹配时把加成计入攻击。

知识项（`0x3919E`）：遍历全部参与者，满足 `knowledge > 80`（不含等号）、`hp > 0`、未退场者才参与；与**攻击方**同阵营的计入攻击项，其余计入防御项，累加值均为 `knowledge * 2`。阵营相对攻击方判定，敌方攻击时两项互换。`Warfield::recalcKnowledge` 维护两侧的知识和，`makeDamage` 按 `ch->side` 取用。

### 5.3 伤害（`0x39391`）

```text
dmg = (实际攻击 - 实际防御 * 3) * 2 / 3
dmg += rnd(20)
dmg -= rnd(20)
若 dmg <= 0：
    dmg = 实际攻击 / 10
    dmg += rnd(4)
    dmg -= rnd(4)
若 dmg < 0：
    dmg = 0                       并跳过后续加成
否则：
    dmg += 攻击方 stamina / 15
    dmg += 目标 hurt / 20
    dmg = 距离衰减(dmg, 距离)
dmg = max(dmg, 1)
```

要点：

- 随机消耗是 2 次或 4 次，取决于是否走回退分支。
- `dmg == 0` 会继续执行体力、受伤与距离衰减，只有 `dmg < 0` 才跳过后续加成。
- 下限钳制在**衰减之后**，因此 1 点伤害经远距离衰减后仍是 1，不会变成 0。

`battle::calcDamage` 实现该式。`battle::predictDamage` 是去掉两组随机项的兼容门面，仅供测试和旧调用能力保留；生产 AI 与界面流程均不调用它。

### 5.4 距离衰减（`0x3944C`）

```text
距离 > 10：dmg = dmg * 2 / 3
否则：     dmg = dmg * (100 - (距离 - 1) * 3) / 100
```

衰减**无条件执行**。距离 1 的系数是 100%，距离 0 的系数是 103%。`battle::applyDistanceDecay` 实现该式。

### 5.5 命中后的状态变化

```text
经验（0x39493）：攻击方 exp += dmg / 5
击杀（0x394DF）：目标 hp - dmg 严格小于 0 时，攻击方 exp += 目标 level * 10
受伤（0x394EE）：目标 hurt += dmg / 10，上限 99
中毒（0x39529）：
    毒力 = (skillLevel / 100 + 1) * skill.addPoison + 攻击方 poisonAmp
    毒力 > 目标 antipoison 且目标 antipoison < 90 时：
        目标 poisoned += (毒力 - antipoison) / 15
```

中毒一步用的是**未经内力降级的原始等级**，与伤害用的等级可能不同；目标已阵亡时同样执行。击杀奖励只在「过量击杀」时给出，伤害恰好等于剩余生命时不奖励。

`battle::poisonOnHit` 实现毒力计算，其余写入在 `mem::actDamage`。

**未复刻**：原版把中毒值与 100 比较却写入 99（`0x395B6`），恰好等于 100 的中毒值因此会保留；本实现统一钳制到 99。

### 5.6 熟练度与体力（`ATK-EXEC`）

```text
双击次数（0x378F1）：doubleAttack == 1 时为 2，否则为 1；两次共用同一目标
熟练度（0x38380）：每次攻击 skillLevel += rnd(2) + 1，上限 999，升级时保留余量
体力（0x38579）：全部攻击次数结束后统一扣 3
```

`mem::postDamage` 按「熟练度成长 → 内力扣除 → 体力扣除」的顺序执行，体力只在最后一次攻击时传入非零值。

### 5.7 吸内力（`0x395EC`）

`skill.damageType > 0` 的武功走这条分支，使用**原始等级** `skillLevel / 100`：

```text
施放者 mp    += skill.addMp[等级]
施放者 maxMp += rnd(skill.addMp[等级] / 2)        上限 999
施放者 mp    += rnd(3) - rnd(3)                   然后钳到 maxMp
目标   mp    -= skill.drainMp[等级] + rnd(3) - rnd(3)   下限 0
返回值 = 目标实际损失的内力
```

施放者的收益与目标的损失**互不相关**，且会永久提升内力上限。这条分支不给经验。

### 5.8 辅助行动

作用范围一律是 `熟练度 / 15 + 1`（`0x397A7`、`0x39B50`、`0x39EB9`、`0x3A33F`）。

| 行动 | 公式 | 体力 | 经验 |
| --- | --- | ---: | ---: |
| 用毒（`0x39A45`） | `(poison - antipoison) / 4`，钳制 0..99 | 2 | 1 |
| 解毒（`0x39DA3`） | `depoison / 3 + rnd(10) - rnd(10)`，钳制 0..99；`目标 poisoned > depoison + 20` 时归零；不超过目标当前中毒值 | 2 | 1 |
| 医疗（`0x3A10C`） | `hurt <= 25` 取 `medic * 4 / 5`，`<= 50` 取 `medic * 3 / 4`，`<= 75` 取 `medic * 2 / 3`，否则 `medic / 2`，再 `+ rnd(5)`；同时 `hurt -= medic` | 4 | 1 |
| 暗器（`0x3A537`） | 见下 | 0 | 0 |
| 休息（`0x3A8A4`） | 见 5.10 | — | 0 |

医疗的 `hurt > medic + 20` 判定在随机数抽取**之后**才生效，此时治疗量与受伤削减同时归零，但随机数已经消耗。实现上由 `battle::medicHeal` 只返回治疗量、`mem::actMedic` 施加拒绝条件来保证这一点。

用毒返回的是实际增加的中毒值（正数），解毒返回实际减少量（正数）。

医疗的 4 点体力由两部分组成：`actMedic` 内部扣 2（`0x3A28E`），三项辅助行动的公共尾段再扣 2（`0x399FE`）。公共尾段同时给 1 点经验，无论行动是否生效都会执行。

### 5.9 暗器（`0x3A537`、`0x3A73B`）

```text
按目标受伤分档取基数：
    hurt == 0   → addHp / 4
    hurt <= 33  → addHp / 3
    hurt <= 66  → addHp / 2
    否则        → addHp
基数 -= rnd(5)
delta = (基数 - 施放者 throwing * 2) / 3

目标 hurt -= delta / 4          钳制 0..99
目标 hp   += delta              钳制 0..maxHp
```

`addHp` 是物品原始值，攻击类暗器为负数，`delta` 随之也为负，扣血的同时抬高受伤值。暗器**没有最低 1 点伤害**的下限。四个分档都消耗一次 `rnd(5)`。

附加毒性：

```text
addPoisoned > 0：
    antipoison >= 100 → 0
    否则 目标 poisoned += max(0, (addPoisoned - 施放者 throwing) / 2 - antipoison) / 2
addPoisoned <= 0：
    目标 poisoned += addPoisoned / 2 + rnd(5) - rnd(5)
```

毒性分支不消耗随机数，解毒分支消耗两次。`battle::throwDamage` 与 `battle::throwPoison` 实现两式。

### 5.10 休息（`0x3A8A4`）

```text
stamina += rnd(3) + (本回合已移动 ? 2 : 3)      上限 100
新 stamina >= 30 时：
    hp += rnd(新 stamina / 10 - 2) + 3          上限 maxHp
    mp += rnd(新 stamina / 10 - 2) + 3          上限 maxMp
```

`rnd` 的参数可能退化到 0 或 1，此时不消耗随机数且返回 0，恢复量固定为 3。

**有意偏离**：原版用剩余步数与 `speed / 10` 比较来判断是否移动过（`0x3A8CF`），而回合发放的步数是 `speed / 15 - hurt / 40`，两者几乎不可能相等，「未移动」奖励因此永远拿不到。本实现按 `remainingSteps != initialSteps` 判断；只要消耗过任意一步，就视为本回合已经移动。

`battle::restGain` 返回三项增量，`mem::actRest` 负责写入与钳制。

### 5.11 升级（`0x3B6BE`）

一次结算全部等级差 `n`，不逐级循环。

```text
成长系数：factor = rnd(档位) + 1
    potential < 30 → 档位 2
    potential < 50 → 档位 3
    potential < 70 → 档位 4
    potential < 90 → 档位 5
    否则           → 档位 6

maxHp += n * 3 * (hpAddOnLevelUp + rnd(3))     上限 999，随后 hp 回满、hurt 与 poisoned 清零、stamina 置 100
maxMp += n * 4 * (9 - factor)                  上限 999，随后 mp 回满
attack、speed、defence += n * factor           各自上限 100

medic、poison、depoison、fist、sword、blade：值 > 20 时各 += rnd(3)
throwing：无条件 += rnd(3)
special：不成长
上述熟练度上限均为 100
```

随机消耗顺序：成长系数 1 次，`maxHp` 的 `rnd(3)` 1 次，六项熟练度按 medic、poison、depoison、fist、sword、blade 的顺序（各自满足条件时），最后 throwing 1 次。

`battle::levelUpFactor` 与 `battle::levelUpGain` 实现前两部分，`mem::actLevelup(c, gainedLevels)` 负责写入。调用方须先算出等级差，不能在循环中反复调用。

## 6. 范围与目标

### 6.1 距离网格（`0x36E7F`）

目标选择与施术范围使用地形距离图：自身格为 0，向四邻逐格扩散，绕开阻挡格（建筑与不可通行地形），**不受角色占位影响**。`battle::terrainPathDistance` 与 `battle::getCastRangeArea` 实现该规则；范围候选格最终仍排除其他角色占位。

实际移动使用 `battle::getSelectableArea` 和占位感知的 `battle::shortestPathDistance`。目标敌人的占位格可以作为终点进入，其他角色占位格不能作为中途路径。目标评分、施术范围和移动可达性因此使用三套明确分开的判断。

目标选择的距离上限**包含等号**：`距离 <= 范围` 即可选中。

### 6.2 四种作用范围

| `attackAreaType` | 形状 | 遍历顺序 | 距离取法 |
| ---: | --- | --- | --- |
| 0 | 单体 | 仅光标格 | 施放者到目标格的曼哈顿距离 |
| 1 | 直线 | 朝向固定，步距由 1 递增到 `selRange[level]` | 步距本身 |
| 2 | 十字 | 步距由 1 递增，每步按上、下、左、右四方向 | 步距本身 |
| 3 | 区域 | 以光标为中心、半径 `area[level]` 的方形，先列后行 | 施放者到**实际命中格**的曼哈顿距离 |

要点：

- 直线与十字都是由近及远遍历，随机数消耗顺序随之固定。
- 十字的四方向顺序是上、下、左、右（`0x37E43`、`0x37FC5`、`0x380C8`、`0x381CD`）。
- 区域武功的距离不是「光标距离 + 偏移」，而是每格单独计算。
- 直线武功不因中途有目标而中断。
- 目标选择范围用 `selRange[level]`，区域武功的伤害半径用 `area[level]`。
- 单体与区域武功只在第一次攻击时选目标，双击的第二次沿用同一目标。

实现在 `Warfield::startActAction` 的 `switch (skillInfo->attackAreaType)`。

### 6.3 辅助行动的范围

用毒、解毒、医疗、暗器的可选距离都是 `对应熟练度 / 15 + 1`。玩家行动由 `Warfield::tryUseSkill` 适配，自动行动统一经过 `Warfield::autoAction` 的施术规划。

## 7. AI

AI 分三段：**决定做什么**（`src/battle/ai_policy.*` 与 `ai_strategy.*`）、**决定打谁**（`ai_strategy.*`）、**怎么走过去并执行**（`src/scene/warfield.cc`）。`src/battle/ai.*` 只把旧的 `AiContext` 接口映射到拆分模块，不复制规则。

### 7.1 快照

`Warfield::autoAction` 在每名角色决策开始时建立纯数据快照：

| 字段 | 内容 |
| --- | --- |
| `AiResourceState` | 生命、受伤、中毒、体力、内力和医疗/解毒能力 |
| `AiAllyState` | 阵营、存活状态、行动码（`8/9` 为请求标记）和攻击，用于资源请求与支援回退 |
| `AiStrategyActor` | 行动者的生命、攻击、体力、内力、能力、integrity 和 potential，用于后续行动与目标策略 |
| `AiStrategyCharacter` | 候选角色的坐标、生命、能力、攻击和毒值；离场槽标记为 inactive |
| 物品快照 | 敌方按 4 个携带槽，玩家按背包保存槽顺序扫描 |
| 武功快照 | 保留原始槽位与熟练度；站位按存储熟练等级规划，执行时再按当前 MP 降级 |

AI 属性快照在 `putChars()` 中于 `mem::addUpPropFromEquipToChar()` 之前建立。装备加成仍保留在 `CharInfo::info`，供实际伤害、速度、防御和施术执行使用；AI 规划从运行时基础字段中扣除入场装备增量。若战斗内直接改写这些运行时字段，下一次决策会保留该改写，不会重新从持久存档读取。

### 7.2 决策级联（`0x33599`）

八级按顺序执行。除第 1 级只记录休息回退外，其余级别一旦产生可执行行动即停止。级联的随机消耗顺序必须与下表一致。

| 级 | 触发条件 | 结果 |
| ---: | --- | --- |
| 1 | `stamina < 10` | 先把行动码设为 `7`（休息回退）；生命恢复流程若被触发，其返回值会覆盖该回退 |
| 2 | `hp < 20` 或 `hurt > 50` 或 `hp < maxHp/2 且 rnd(10) < 3` 或 `hp < maxHp/3 且 rnd(10) < 5` 或 `hp < maxHp/4 且 rnd(10) < 7` 或 `hp < maxHp/5 且 rnd(10) < 9` | 走补血流程（7.3） |
| 3 | `rnd(10) < poisoned / 10`（无条件抽取） | 走解毒流程（7.3） |
| 4 | `mp < maxMp/2 且 rnd(10) < 2` 或 `mp < maxMp/3 且 rnd(10) < 4` 或 `mp < maxMp/4 且 rnd(10) < 6` 或 `mp < maxMp/5 且 rnd(10) < 8` | 找 `addMp > 0` 的物品 |
| 5 | `stamina > 50` 且（`medic >= 20 且 rnd(10) < 4` 或 `medic >= 40 且 rnd(10) < 6` 或 `medic >= 60 且 rnd(10) < 8` 或 `medic >= 80`） | 为友方医疗（7.4） |
| 6 | `stamina > 50` 且 `depoison` 满足同型四段阈值 | 为友方解毒（7.4） |
| 7 | `rnd(10) < 5` 且（`hp < 20` 或 `hp < maxHp/4 且 rnd(10) < 6` 或 `hp < maxHp/5 且 rnd(10) < 8`） | 逃跑 |
| 8 | 其余 | 行动选择（7.5） |

第 1 级不消耗随机数。第 2 级与第 7 级的条件按顺序逐个判断，前面条件成立时，后面的随机数不再抽取。

低体力只会先记录 `Rest` 候选，不会立即结束级联。若第 2 级生命恢复条件命中，`0x33C4D` 的返回值会覆盖该候选；自身医疗、物品和队友请求都无法执行时，返回 0 会清除休息候选，并继续解毒、补内力和支援判断。

### 7.3 自救与求助（`0x33C4D`、`0x33E93`、`0x340D9`）

补血流程分三级：

```text
1. medic >= 20 且 stamina >= 50 且 medic > hurt - 30      → 对自己医疗
2. 物品中存在 addHp > 0                                   → 使用该物品
3. 存在同阵营在场友方，medic > 20 且 medic > 我的 hurt - 30 → 在自身 `actionCode` 写入 `8`，本回合照常攻击
```

解毒流程依次尝试自身解毒、物品和队友请求。自身解毒严格要求 `depoison > 20`、`stamina > 50` 且 `depoison > poisoned - 30`；物品判据是 `addPoisoned < 0`。补内力只有一步：找 `addMp > 0` 的物品。

场景适配把纯 AI 门面的 `AiRequest::Medic` / `AiRequest::Depoison` 写成行动者自己的 `actionCode == 8/9`。之后行动的友方若具备对应熟练度，看到该标记会跳过全部概率门直接施救。

**有意偏离**：原版己方分支筛的是 `addPoison < 0`（用毒熟练度修正）而不是 `addPoisoned`，导致己方自动战斗永远找不到解毒药；敌方分支用的是正确字段。本实现两侧统一使用 `addPoisoned`。

请求行动码在角色自己下次行动开始时清零（`0x329D0`），因此可以跨回合保留到该角色再次行动之前。

### 7.4 为友方医疗与解毒（`0x341F6`、`0x343DA`）

按参与者下标顺序取**第一个**满足条件者，不选「最严重」的：

```text
医疗：前置 我的 medic > 对方 hurt - 30
      条件 对方 `actionCode == 8`
        或 hp < 20
        或 hurt > 40
        或 hp < maxHp/2 且 rnd(10) < 7
        或 hp < maxHp/3 且 rnd(10) < 8
        或 hp < maxHp/4 且 rnd(10) < 9
        或 hp < maxHp/5

解毒：前置 我的 depoison > 对方 poisoned - 30
      条件 对方 `actionCode == 9`
        或 poisoned > 10 且 rnd(10) < 4
        或 poisoned > 20 且 rnd(10) < 6
        或 poisoned > 30 且 rnd(10) < 8
        或 poisoned > 40
```

### 7.5 行动选择（`0x34550`）

```text
1. 战力对比转辅助：
     敌方战力均值 / 2 > 我的 hp + attack
     且 己方战力总和 > 敌方战力总和 * 2
   成立时：medic >= 20 且 stamina >= 50 → 医疗生命缺口最大的友方
           否则 depoison >= 20 且 stamina >= 50 → 为中毒值最高的友方解毒
   战力 = 该阵营全部参与者的 attack + hp 之和，统计时包含不在场者

2. 用毒：poison - attack > rnd(50) 且 rnd(150) < poison

3. 投掷：扫描物品
     己方：|addHp| > attack * 3 / 2 且 rnd(throwing) > 20
     敌方：|addHp| > attack 且 rnd(10) < 6
     毒性物品：addPoisoned > 同一门槛 且 rnd(10) < 3

4. 普通攻击：stamina > 10 且 mp >= 已学武功中最小的 reqMp

5. 以上都不成立 → 休息
```

### 7.6 目标选择（`0x3505B`）

按人物性格分流，各有 70% 触发率，依次尝试；某个分支触发但没有合格候选时继续检查后续分支：

| 条件 | 选择 |
| --- | --- |
| `integrity >= 75` 且 `rnd(10) < 7` | 攻击最高的在场敌人 |
| `integrity <= 25` 且 `rnd(10) < 7` | 攻击最低的在场敌人 |
| `potential >= 70` 且 `rnd(10) < 7` | 敌方辅助：己方有 `poison > 20` 者时取解毒最高的敌人，否则取医疗最高的敌人，均要求该熟练度 >= 20；都不满足时退回「攻击最低」 |
| 其余 | 洪泛距离最近的在场敌人 |

用毒目标另有一套（`0x355FF`），只考虑 `poisoned < 95` 且 `antipoison < 我方 poison` 的在场敌人：`potential > 60` 且 `rnd(10) < 7` 时取攻击最高者，否则取距离最近者。

高资质能力分支保留原版的标志缺陷：己方存在 `poison > 20` 角色且先找到敌方解毒者时，若后续医疗扫描没有设置成功标志，仍回退到「攻击最低」目标。

**有意偏离与兼容边界**（三处）：

- 原版 `sub_3505B` 的高、低性格辅助函数在没有候选时可能保留上一行动的 `word_DC742`；本实现把空结果视为该分支失败，继续检查后续策略，最后才使用最近目标，避免复用过期目标。
- 原版最近目标扫描 `sub_35372` 以 `1000` 为初始距离，并只接受严格更小的结果；本实现保持该上限，距离 `>= 1000` 的敌人不参与最近目标选择。
- 原版用毒目标的距离比较读的是上一次记录的目标而非当前候选（`0x357AB`），所有候选拿到同一距离，实际总取第一个；本实现按候选自身距离比较。

### 7.7 攻击执行（`0x34C47`）

```text
1. 在 skillId > 0 的槽位中随机选一门（不做伤害评估）
2. 以存储熟练度计算规划等级和射程 `selRange[level]`；执行时再以当前 MP 确定实际等级
3. 用 7.6 取目标
4. 判定能否命中：
     attackAreaType 0 或 3：距离 <= 射程
     attackAreaType 1 或 2：距离 <= 射程 且 与目标同排或同列
5. 能命中 → 出手
   否则剩余步数 > 0 时移动（7.8），移动后重判
   仍不能命中 → 改取最近的敌人重判
   仍不能命中 → 休息
```

`Warfield::autoAction` 实现该流程。首个目标最多安排一次移动；移动完成后重新检查首个目标，失败时只在当前位置检查最近目标，不能为备用目标再次安排移动。若当前距离不在范围内且本回合仍有步数但无法抵达范围，先移动到可达 frontier，再在移动结束后复查；没有前进格时不把原地格当作有效移动。

### 7.8 移动（`0x3650E`、`0x34AEC`）

进攻站位：从射程上限向下枚举期望距离 `r`，在本回合走得到的格子中找地形距离恰好为 `r` 的格子（直线与十字还要求与目标同排同列），取离当前位置曼哈顿距离最小的一格；以第一个找到候选格的 `r` 为准。普通接近使用 `CastMovementMode::Approach`，直线/十字使用 `Aligned`。用毒在有剩余步数时使用 `CastMovementMode::Reposition`，优先保持最大允许站距；无步数时只复查当前位置，不安排第二次移动。

如果本回合不能进入施术范围，仍先移动到可达 frontier；移动后只复查一次，失败即进入回退。没有任何前进格时不把原地格视为移动成功。用毒在有剩余步数时使用 `CastMovementMode::Reposition`，从最大允许站距向下寻找；已在最大站距时允许原地施毒。

撤退站位（逃跑与用物品前）：只在**恰好用尽全部移动力**的格子中挑选，取与敌方各角色曼哈顿距离之和最大的一格。逃跑后休息，用物品前的撤退不休息。

**有意偏离**：原版从目标位置重建移动格图并扫描整张 64×64 地图，不检查候选格是否走得到，可能选出本回合到不了的落点。本实现只枚举本回合真正走得到的格子；枚举顺序（先 `x` 后 `y`）与同分取先者的规则与原版一致。

### 7.9 行动映射

`battle::AiAction` 与原版 12 项行动码的对应：

| `AiAction` | 原版码 | 执行入口 |
| --- | ---: | --- |
| `Rest` | 0、7 | `Warfield::doRest` |
| `Attack` | 1、2、8、9 | `Warfield::autoAction` 的普通武功路径 |
| `Poison` | 3 | `Warfield::autoAction` 的用毒路径 |
| `Depoison` | 4 | `Warfield::autoAction` 的解毒路径 |
| `Medic` | 5 | `Warfield::autoAction` 的医疗路径 |
| `UseItem` | 6 | 撤退后 `Warfield::autoUseItem` |
| `Throw` | 10 | `Warfield::autoThrow` |
| `Flee` | 11 | 撤退后休息 |

辅助行动无法送达时（目标不可达），按原版 `0x36366` 的规则回退：`attack * 2 > 己方战力总和 * 2 / 人数` 时改为普通武功，否则休息。直接支援的行动码 `4/5` 保留；请求者的 `8/9` 只有在确实安排接近移动后才保留续行动标记。`pendingAutoAction_` 执行前先移出回调，回调内部新建的待执行动作不会被外层清除。

## 8. 战后结算

### 8.1 结算顺序（`0x3B387`）

```text
1. 敌方角色 hp 与 mp 回满，stamina 置 100，hurt 与 poisoned 清零
2. 胜利时把 战场经验 / 存活人数 加到每个存活的己方角色
3. 己方存活者 hp 不低于 maxHp / 5
   己方倒下者 hp 置 maxHp / 5，stamina 不低于 10
4. 对每个角色（不看胜负）：
     exp             += 本场经验
     expForItem      += 本场经验 * 8 / 10
     expForMakeItem  += 本场经验 * 8 / 10
     三者上限 60000
5. 胜利或「失败也给经验」开关打开时，才显示经验提示并依次执行升级、练功、制药
```

第 4 步的三项经验各自独立累加，不是在「升级」与「练功」之间二选一。

实现在 `Warfield::endWar`。战斗期间的状态改动只作用于 `CharInfo::info` 副本，因此第 1 步在本实现中天然成立（敌方数据从不回写）。

### 8.2 升级

见 5.11。调用方须先用 `mem::getExpForLevelUp` 算出等级差，再一次性调用 `mem::actLevelup(charInfo, gained)`。

### 8.3 资质档位

练功与制药共用同一个档位（`0x3BB21`、`0x3C2E1`）：

```text
tier = 7 - potential / 15          不做钳制，取值范围 1..7
```

资质越高档位越小，需要的经验越少。`battle::potentialTier` 实现该式。

### 8.4 练功（`0x3BA85`）

```text
tier = potentialTier(potential)
若 item.skillId != -1：
    level = 该武功在角色身上的当前等级（未学得时为 0）
    need  = (level + 1) * item.reqExp * tier
    level >= 9 时整步跳过
否则（纯属性书）：
    need = item.reqExp * tier * 2

learningItem == -1 时跳过
expForItem < need 时跳过

执行内容（每场战斗最多一次）：
    套用书本属性（见下表）
    expForItem = 0
    若 item.skillId > 0：
        已学得该武功 → 存储值 < 899 时 skillLevel += 100
        未学得       → 放进第一个空槽位，等级不变
```

要点：

- **每场战斗只推进一次**，不循环。
- `expForItem` 直接**清零**，不是减去 `need`，超出部分不保留。
- 升级是 `+= 100`，余量随之带入下一级，不做取整。
- 全程**不消耗随机数**，也没有内力上限的随机加成。

书本属性套用（`0x3BC6C`）是一套**独立规则**，与消耗品用的 `mem::applyItemChanges` 不同，按下列顺序执行 19 项：

| 物品字段 | 目标 | 规则 |
| --- | --- | --- |
| `addMaxHp` | `maxHp` | `+=`，只有上限 999，没有下限 |
| `changeMpType` | `mpType` | 仅当 `changeMpType == 2` 时把 `mpType` 置 2，其他取值不动 |
| `addMaxMp` | `maxMp` | `+=`，只有上限 999，没有下限 |
| `addAttack` … `addIntegrity` | `attack`、`speed`、`defence`、`medic`、`poison`、`depoison`、`antipoison`、`fist`、`sword`、`blade`、`special`、`throwing`、`knowledge`、`integrity` | `+=` 后钳进 0..100，先判上限，再判下限 |
| `addDoubleAttack` | `doubleAttack` | 仅当角色当前为 0 时**赋值**（不是相加） |
| `addPoisonAmp` | `poisonAmp` | `+=` 后钳进 0..100 |

`addHp`、`addMp`、`addPoisoned`、`addStamina` 这四个消耗品字段**完全不套用**。

`mem::getExpForSkillLearn` 负责需求量，`mem::applyBookChanges` 负责属性套用（钳制语义由 `battle::applyBookStat` 提供），写入流程在 `Warfield::endWar`。

### 8.5 制药（`0x3C2AC`）

```text
need = item.reqExpForMakeItem * tier
reqExpForMakeItem <= 0 或 expForMakeItem < need 时跳过

材料数量 = 行囊中 item.reqMaterial 的数量
候选配方 = { k | 材料数量 > 0 且 item.makeItem[k] >= 0 且 材料数量 >= item.makeItemCount[k] }
候选为空时跳过

反复 rnd(5) 直到落在候选上，得到 pick
产出数量 = 行囊中已有该产物 ? rnd(3) + 1 : 1
行囊 += (item.makeItem[pick], 产出数量)
行囊 -= (item.reqMaterial, item.makeItemCount[pick])
expForMakeItem = 0
```

要点：

- `makeItemCount[k]` 是该配方**消耗的材料数量**，不是产出数量。
- 产出数量在「行囊已有该物品」与「新开格子」两种情况下不一致，这是原版的写法。
- 抽取范围是全部 5 个配方槽位，不限于排在前面的几个。
- 只有己方角色制药。

`mem::getExpForMakeItem` 负责需求量，其余在 `Warfield::endWar`。

## 9. 差异索引

三类差异的完整清单在 `docs/reverse/battle-behavior-matrix.md`：

- **已修复的差异**：本实现原先与原版不符、现已改正的项。
- **修正版差异**：原版可稳定复现但确认属于缺陷、本实现有意不同的项。本页在对应小节用「有意偏离」标注。
- **未复刻的原版行为**：已确认但本实现仍保留自有逻辑的项。本页在对应小节用「未复刻」标注。

## 10. 修改指引

| 要改的东西 | 动这些文件 | 连带检查 |
| --- | --- | --- |
| 某条数值公式 | `src/battle/formulas.cc`（含 `.hh` 注释与地址） | `formula_tests.cc` 的对应用例、随机消耗次数断言 |
| 随机消耗次数 | 相关规则函数 | 所有用 `SequenceRandom` 的测试都会因序列错位而失败，属预期 |
| 属性上限 | `src/data/consts.hh` | `mem/action.cc` 中所有 `std::clamp` 调用点 |
| AI 判据或阈值 | `src/battle/ai_policy.cc`、`ai_strategy.cc` | `ai_tests.cc`、`ai_policy_tests.cc`、`ai_strategy_tests.cc`；注意级联条件的判断顺序决定随机消耗 |
| AI 执行方式（移动、出手） | `src/scene/warfield.cc`、`src/battle/movement.cc` | `movement_tests.cc`；场景回调仍需实际战斗验证 |
| 作用范围形状或遍历顺序 | `src/battle/attack_area.cc`、`Warfield::startActAction` | `attack_area_tests.cc` 与战场表现分支 |
| 回合流程 | `Warfield::nextAction`、`src/battle/turn_order.hh` | `turn_order_tests.cc` |
| 战后结算 | `Warfield::endWar`、`mem::actLevelup` | 无纯逻辑测试覆盖，需实际战斗验证 |
| 练功或制药的需求量 | `mem::getExpForSkillLearn`、`mem::getExpForMakeItem`、`battle::potentialTier` | `formula_tests.cc` 的 `potentialTiers` 用例 |
| 书本属性套用 | `mem::applyBookChanges`、`battle::applyBookStat` | `formula_tests.cc` 的 `bookStats` 用例；不要顺手改 `applyItemChanges`，那条路径用于消耗品 |

新增一条规则时的约定：纯计算放进 `src/battle/`，状态写入放进 `src/mem/action.cc`，场景交互放进 `src/scene/warfield.cc`；函数注释首行给出证据 ID 与地址。数值公式的一参原版随机经 `battle::originalRandom` 取用；AI 与策略层可直接调用一参 `RandomSource::next`；两参闭区间接口仅按独立契约使用。
