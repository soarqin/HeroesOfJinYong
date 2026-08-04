#include "warfield.hh"

#include "battle/combat_rules.hh"
#include "content/warfielddata.hh"
#include "messagebox.hh"
#include "window.hh"
#include "world/action.hh"
#include "world/bag.hh"
#include "world/savedata.hh"
#include "world/strings.hh"

#include <algorithm>
#include <fmt/xchar.h>
#include <utility>
#include <vector>

namespace hojy::scene {
void Warfield::endWar() {
    bool battleSessionFinished = false;
    if (battleEngine_.status() == battle::EngineStatus::Finished) {
        syncBattleParticipantsToWorking();
        won_ = battleEngine_.snapshot().won;
        battleSessionFinished = true;
    } else if (battleEngine_.status() != battle::EngineStatus::Idle) {
        syncBattleParticipantsToWorking();
        battleEngine_.abort();
        syncBattleParticipantsFromWorking();
        battleParticipants_.clear();
        battleBag_ = {};
        battleBagActive_ = false;
        stage_ = Finished;
        return;
    } else {
        stage_ = Finished;
        return;
    }
    currentActor_ = nullptr;
    pendingAutoAction_ = nullptr;
    movingPath_.clear();
    resumeAutoAttack_ = false;
    clearActionState(false);
    removeAllChildren();
    fadeNode_ = nullptr;
    fadePostAction_ = nullptr;
    runFadePostAction_ = false;
    std::vector<std::pair<std::int16_t,
                          ::hojy::world::state::CharacterData>> stagedCharacters;
    stagedCharacters.reserve(chars_.size());
    for (const auto &ci: chars_) {
        if (ci.side != 0) { continue; }
        auto *persistent = ::hojy::world::state::gSaveData.charInfo[ci.id];
        if (persistent) {
            stagedCharacters.emplace_back(ci.id, *persistent);
        }
    }
    const auto stagedCharacter = [&stagedCharacters](std::int16_t id)
        -> ::hojy::world::state::CharacterData * {
        for (auto &entry: stagedCharacters) {
            if (entry.first == id) { return &entry.second; }
        }
        return nullptr;
    };
    std::vector<CharInfo*> alives;
    for (auto &ci: chars_) {
        if (ci.side != 0) { continue; }
        auto *charInfo = stagedCharacter(ci.id);
        if (!charInfo) { continue; }
        charInfo->maxMp = battle::mergeBattleMaxMpGrowth(
            ci.persistentEntryMaxMp, ci.battleEntryMaxMp, ci.info.maxMp);
        charInfo->mp = std::clamp<std::int16_t>(
            ci.info.mp, 0, charInfo->maxMp);
        charInfo->poisoned = ci.info.poisoned;
        charInfo->hurt = ci.info.hurt;
        charInfo->stamina = ci.info.stamina;
        const auto floorHp = std::int16_t(ci.info.maxHp / 5);
        if (ci.info.hp > 0) {
            charInfo->hp = std::max<std::int16_t>(ci.info.hp, floorHp);
        } else {
            charInfo->hp = floorHp;
            charInfo->stamina = std::max<std::int16_t>(charInfo->stamina, 10);
        }
        for (int i = 0; i < ::hojy::content::LearnSkillCount; ++i) {
            if (ci.info.skillId[i] <= 0) { continue; }
            charInfo->skillLevel[i] = ci.info.skillLevel[i];
        }
        if (ci.info.hp > 0) { alives.push_back(&ci); }
    }
    const auto *info = ::hojy::content::gWarfieldData.info(warId_);
    const auto wexp = info != nullptr ? info->exp : 0;
    std::vector<std::pair<int, std::wstring>> messages = {{0, GETTEXT(won_ ? 93 : 94)}};
    /* The battlefield bonus is victory-only; the loss flag gates the later
     * level/training/crafting steps, not this shared bonus. */
    if (won_ && !alives.empty()) {
        for (auto *ch: alives) {
            ch->exp += wexp / static_cast<int>(alives.size());
        }
    }
    for (auto &ci: chars_) {
        if (ci.side != 0) { continue; }
        auto *ch = &ci;
        auto *charInfo = stagedCharacter(ch->id);
        if (!charInfo) { continue; }
        const int exp = ch->exp;
        const int exp2 = ch->exp * 8 / 10;
        charInfo->exp = std::uint16_t(
            std::clamp<int>(int(charInfo->exp) + exp, 0, ::hojy::content::ExpMax));
        charInfo->expForItem = std::uint16_t(
            std::clamp<int>(int(charInfo->expForItem) + exp2, 0, ::hojy::content::ExpMax));
        charInfo->expForMakeItem = std::uint16_t(
            std::clamp<int>(int(charInfo->expForMakeItem) + exp2, 0, ::hojy::content::ExpMax));
        if (!won_ && !getExpOnLose_) { continue; }

        const auto name = GETCHARNAME(ch->id);
        messages.emplace_back(std::make_pair(0, fmt::format(GETTEXT(95), name, ch->exp)));
        bool canLearn = false;
        bool makingItem = false;
        std::int16_t skillId = 0;
        int skillLevel = 0;
        const ::hojy::world::state::ItemData *itemInfo = nullptr;
        if (charInfo->learningItem >= 0) {
            itemInfo = ::hojy::world::state::gSaveData.itemInfo[charInfo->learningItem];
            if (itemInfo) {
                makingItem = true;
                canLearn = true;
                skillId = itemInfo->skillId;
                if (skillId > 0) {
                    for (int i = 0; i < ::hojy::content::LearnSkillCount; ++i) {
                        if (charInfo->skillId[i] != skillId) { continue; }
                        skillLevel = std::clamp<std::int16_t>(
                            charInfo->skillLevel[i] / 100, 0, ::hojy::content::SkillLevelMaxDiv);
                        if (skillLevel >= ::hojy::content::SkillLevelMaxDiv) { canLearn = false; }
                        break;
                    }
                }
            }
        }

        if (charInfo->level < ::hojy::content::LevelMax) {
            int gained = 0;
            std::uint16_t expReq;
            while (charInfo->level + gained < ::hojy::content::LevelMax
                   && (expReq = ::hojy::world::state::getExpForLevelUp(charInfo->level + gained)) > 0
                   && charInfo->exp >= expReq) {
                ++gained;
            }
            if (gained > 0) {
                ::hojy::world::state::actLevelup(charInfo, gained, battleRandom_);
                messages.emplace_back(std::make_pair(0, fmt::format(GETTEXT(96), name)));
            }
        }

        if (canLearn && itemInfo) {
            const auto expReq = ::hojy::world::state::getExpForSkillLearn(
                charInfo->learningItem, skillLevel, charInfo->potential);
            if (expReq > 0 && charInfo->expForItem >= expReq) {
                ::hojy::world::state::applyBookChanges(charInfo, itemInfo);
                charInfo->expForItem = 0;
                messages.emplace_back(std::make_pair(0, fmt::format(
                    GETTEXT(97), name, GETITEMNAME(charInfo->learningItem))));
                if (skillId > 0) {
                    bool known = false;
                    for (int i = 0; i < ::hojy::content::LearnSkillCount; ++i) {
                        if (charInfo->skillId[i] != skillId) { continue; }
                        known = true;
                        if (charInfo->skillLevel[i] >= ::hojy::content::SkillLevelMaxDiv * 100 - 1) {
                            continue;
                        }
                        charInfo->skillLevel[i] = std::int16_t(charInfo->skillLevel[i] + 100);
                        messages.emplace_back(std::make_pair(1, fmt::format(
                            GETTEXT(98), GETSKILLNAME(skillId),
                            charInfo->skillLevel[i] / 100 + 1)));
                    }
                    if (!known) {
                        for (int i = 0; i < ::hojy::content::LearnSkillCount; ++i) {
                            if (charInfo->skillId[i] > 0) { continue; }
                            charInfo->skillId[i] = skillId;
                            break;
                        }
                    }
                }
            }
        }

        if (makingItem && itemInfo) {
            const auto craftReq = ::hojy::world::state::getExpForMakeItem(
                charInfo->learningItem, charInfo->potential);
            const auto material = battleBag_[itemInfo->reqMaterial];
            bool affordable[::hojy::content::MakeItemCount] = {false};
            bool anyAffordable = false;
            for (int i = 0; i < ::hojy::content::MakeItemCount; ++i) {
                if (itemInfo->makeItem[i] < 0
                    || material < itemInfo->makeItemCount[i]) {
                    continue;
                }
                affordable[i] = true;
                anyAffordable = true;
            }
            if (craftReq > 0 && charInfo->expForMakeItem >= craftReq && anyAffordable) {
                int index;
                do {
                    index = battleRandom_.next(::hojy::content::MakeItemCount);
                } while (!affordable[index]);
                const auto produced = battleBag_[itemInfo->makeItem[index]] > 0
                    ? std::int16_t(battleRandom_.next(3) + 1)
                    : std::int16_t(1);
                charInfo->expForMakeItem = 0;
                battleBag_.add(itemInfo->makeItem[index], produced);
                battleBag_.remove(itemInfo->reqMaterial, itemInfo->makeItemCount[index]);
                messages.emplace_back(std::make_pair(0, fmt::format(
                    GETTEXT(99), name, GETITEMNAME(itemInfo->makeItem[index]))));
            }
        }
    }
    if (battleSessionFinished) {
        for (auto &ci: chars_) {
            if (ci.side != 0) { continue; }
            if (auto *charInfo = stagedCharacter(ci.id)) {
                ci.info = *charInfo;
            }
        }
        syncBattleParticipantsToWorking();
        if (battleEngine_.reconcile(battleInventorySnapshot())) {
            const auto result = battleEngine_.finish(true);
            if (result.committed) {
                won_ = result.won;
                syncBattleParticipantsFromWorking();
                battleParticipants_.clear();
                for (const auto &[id, character]: stagedCharacters) {
                    if (auto *persistent = ::hojy::world::state::gSaveData.charInfo[id]) {
                        *persistent = character;
                    }
                }
                commitBattleBag();
            } else {
                syncBattleParticipantsFromWorking();
                battleParticipants_.clear();
                battleBag_ = {};
                battleBagActive_ = false;
            }
        } else {
            battleEngine_.abort();
            syncBattleParticipantsFromWorking();
            battleParticipants_.clear();
            battleBag_ = {};
            battleBagActive_ = false;
        }
    }
    stage_ = Finished;
    popupFinishMessages(std::move(messages), 0);
    // The status panel is a renderer-owned root node, not a child of the
    // battlefield tree. Delete it through its explicit owner at the update
    // barrier instead of queueing a request that Window cannot observe.
    delete statusPanel_;
    statusPanel_ = nullptr;
}

void Warfield::popupFinishMessages(std::vector<std::pair<int, std::wstring>> messages, int index) {
    int y = height_ / 3;
    auto *msgBox = new MessageBox(this, 0, y, width_, 60);
    msgBox->popup({messages[index].second}, MessageBox::PressToCloseThis);
    ++index;
    auto *lastMsgBox = msgBox;
    while (index < messages.size() && messages[index].first > 0) {
        auto *msgBox2 = new MessageBox(msgBox, 0, y + 60 * messages[index].first, width_, 60);
        msgBox2->popup({messages[index].second}, MessageBox::PressToCloseParent);
        lastMsgBox = msgBox2;
        ++index;
    }
    lastMsgBox->setCloseHandler([this, messages = std::move(messages), index]() {
        if (index < messages.size()) {
            popupFinishMessages(messages, index);
        } else {
            const bool won = won_;
            const bool instantDie = !won && deadOnLose_;
            cleanup();
            gWindow->endWar(won, instantDie);
        }
    });
}

}
