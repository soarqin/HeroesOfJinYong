#include "warfield.hh"

#include "battle/attack_area.hh"
#include "battle/combat_rules.hh"
#include "content/constants.hh"
#include "effect.hh"
#include "menu.hh"
#include "messagebox.hh"
#include "window.hh"
#include "world/action.hh"
#include "world/bag.hh"
#include "world/savedata.hh"
#include "world/strings.hh"

#include <algorithm>
#include <fmt/xchar.h>
#include <functional>
#include <utility>

namespace hojy::scene {
class DirectionSelMessageBox: public MessageBox {
public:
    using MessageBox::MessageBox;

    void setDirectionHandler(const std::function<void(Map::Direction)> &func) {
        directionHandler_ = func;
    }
    void handleKeyInput(Key key) override {
        switch (key) {
        case KeyUp:
            directionHandler_(Map::DirUp);
            requestDelete();
            break;
        case KeyLeft:
            directionHandler_(Map::DirLeft);
            requestDelete();
            break;
        case KeyRight:
            directionHandler_(Map::DirRight);
            requestDelete();
            break;
        case KeyDown:
            directionHandler_(Map::DirDown);
            requestDelete();
            break;
        case KeyCancel: {
            auto fn = std::move(closeHandler_);
            requestDelete();
            if (fn) { fn(); }
            break;
        }
        default:
            break;
        }
    }

private:
    std::function<void(Map::Direction)> directionHandler_;
};

bool Warfield::tryUseSkill(int index) {
    auto *ch = currentActor_;
    if (!ch) { return false; }
    clearActionState(false);
    if (index < 0) {
        actIndex_ = -1;
        actId_ = index;
        actLevel_ = 0;
        attackTimesLeft_ = 1;
        int steps;
        switch (index) {
        case -3:
            steps = battle::calcTechniqueRange(ch->info.poison);
            break;
        case -2:
            steps = battle::calcTechniqueRange(ch->info.depoison);
            break;
        case -1:
            steps = battle::calcTechniqueRange(ch->info.medic);
            break;
        default:
            steps = 1;
            break;
        }
        maskSelectableArea(0, steps);
        stage_ = AttackSelecting;
        drawDirty_ = true;
        return true;
    }
    const auto *skill = ::hojy::world::state::gSaveData.skillInfo[std::max<std::int16_t>(ch->info.skillId[index], 0)];
    if (!skill) { return false; }
    auto skillLevel = std::clamp<std::int16_t>(ch->info.skillLevel[index] / 100, 0, 9);
    skillLevel = ::hojy::world::state::calcRealSkillLevel(skill->reqMp, skillLevel, ch->info.mp);
    if (skillLevel < 0) { return false; }
    actIndex_ = index;
    actId_ = ch->info.skillId[index];
    attackTimesLeft_ = battle::attackCount(ch->info.doubleAttack);
    actLevel_ = skillLevel;
    switch (skill->attackAreaType) {
    case 1: {
        auto msgBox = new DirectionSelMessageBox(this, 0, 0, gWindow->width(), gWindow->height());
        msgBox->popup({GETTEXT(92)});
        msgBox->setCloseHandler([this]() {
            clearActionState(false);
            playerMenu();
        });
        msgBox->setDirectionHandler([this, ch](Map::Direction direction) {
            ch->direction = direction;
            startActAction();
        });
        return true;
    }
    case 2:
        startActAction();
        return true;
    default:
        maskSelectableArea(0, skill->selRange[actLevel_]);
        stage_ = AttackSelecting;
        drawDirty_ = true;
        return true;
    }
}

void Warfield::startActAction() {
    popupNumbers_.clear();
    auto *ch = currentActor_;
    if (!ch) { stage_ = Idle; return; }
    if (actId_ < 0) {
        if (cursorX_ < 0 || cursorX_ >= mapWidth_
            || cursorY_ < 0 || cursorY_ >= mapHeight_) {
            if (ch->side == 0) {
                clearActionState(false);
                playerMenu();
            }
            else { endTurn(ch); }
            return;
        }
        auto *target = cellInfo_[cursorY_ * mapWidth_ + cursorX_].charInfo;
        if (!target) {
            if (ch->side == 0) {
                clearActionState(false);
                playerMenu();
            }
            else { endTurn(ch); }
            return;
        }
        const auto recordNoOp = [&]() {
            if (const auto actor = participantIndex(ch)) {
                if (!recordBattleAction(battle::BattleAction{
                        *actor,
                        battle::NoOpAction{static_cast<std::int8_t>(actId_)},
                    })) {
                    return false;
                }
            }
            endTurn(ch);
            return true;
        };
        const bool targetAlive = target->info.hp > 0;
        const bool targetIsEnemy = target->side != ch->side;
        const bool targetIsAlly = !targetIsEnemy;
        const auto *itemInfo = actId_ == -4 && actIndex_ >= 0
            && static_cast<std::size_t>(actIndex_) <
                ::hojy::world::state::gSaveData.itemInfo.size()
            ? ::hojy::world::state::gSaveData.itemInfo[actIndex_]
            : nullptr;
        const bool validTarget = actId_ == -3
            ? targetAlive && targetIsEnemy
            : actId_ == -2 || actId_ == -1
                ? targetAlive && targetIsAlly
                : actId_ == -4 && itemInfo && targetAlive && targetIsEnemy;
        if (!validTarget) {
            recordNoOp();
            return;
        }
        std::int16_t result;
        std::uint8_t r, g, b;
        auto *ttf = renderer_->ttf();
        bool popup;
        bool actionValid = false;
        switch (actId_) {
        case -3:
            effectId_ = ::hojy::content::PoisonEffectID;
            actionValid = targetAlive && target->side != ch->side;
            popup = target && target->side != ch->side;
            result = popup ? ::hojy::world::state::actPoison(&ch->info, &target->info, 0) : 0;
            popup = popup && result != 0;
            r = 96; g = 176; b = 64;
            break;
        case -2:
            effectId_ = ::hojy::content::DepoisonEffectID;
            actionValid = targetAlive && target->side == ch->side;
            popup = target && target->side == ch->side;
            result = popup ? ::hojy::world::state::actDepoison(
                &ch->info, &target->info, 0, battleRandom_) : 0;
            r = 104; g = 192; b = 232;
            break;
        case -1:
            effectId_ = ::hojy::content::MedicEffectID;
            actionValid = targetAlive && target->side == ch->side;
            popup = target && target->side == ch->side;
            result = popup ? ::hojy::world::state::actMedic(
                &ch->info, &target->info, 2, battleRandom_) : 0;
            r = 236; g = 200; b = 40;
            break;
        default: {
            effectId_ = itemInfo ? itemInfo->throwingEffectId : ::hojy::content::PoisonEffectID;
            actionValid = itemInfo && targetAlive && target->side != ch->side;
            popup = target && target->side != ch->side;
            bool dead = false;
            result = popup ? ::hojy::world::state::actThrow(
                &ch->info, &target->info, actIndex_, 0, dead,
                battleRandom_) : 0;
            if (popup) {
                if (ch->side == 0) { battleBag_.remove(actIndex_, 1); }
                else {
                    ::hojy::world::state::consumeNpcItemAt(&ch->info, actItemSlot_, actIndex_);
                }
            }
            popup = popup && result != 0;
            if (dead) {
                recalcKnowledge();
            }
            r = 232; g = 32; b = 44;
            break;
        }
        }
        if (popup) {
            auto txt = fmt::format(L"{:+}", result);
            popupNumbers_.emplace_back(PopupNumber{txt, cursorX_, cursorY_, r, g, b});
        }
        if (actId_ >= -3 && actId_ <= -1) {
            battle::finishUtilityAction(ch->info, ch->exp);
        }
        if (actionValid) {
            const auto actor = participantIndex(ch);
            const auto targetIndex = participantIndex(target);
            if (actor && targetIndex) {
                battle::Technique technique = battle::Technique::Poison;
                if (actId_ == -2) {
                    technique = battle::Technique::Depoison;
                } else if (actId_ == -1) {
                    technique = battle::Technique::Medic;
                }
                const battle::BattleAction action{
                    *actor,
                    actId_ == -4
                        ? battle::ActionPayload{battle::ThrowAction{
                              *targetIndex, actIndex_,
                              ch->side == 0
                                  ? battle::InventorySource::PartyBag
                                  : battle::InventorySource::NpcCarry,
                              static_cast<std::int16_t>(
                                  ch->side == 0 ? -1 : actItemSlot_),
                          }}
                        : battle::ActionPayload{battle::TechniqueAction{
                              technique, *targetIndex,
                          }},
                };
                if (!recordBattleAction(action)) { return; }
            }
        }
        stage_ = Acting;
        if (cameraX_ != cursorX_ || cameraY_ != cursorY_) {
            ch->direction = calcDirection(cameraX_, cameraY_, cursorX_, cursorY_);
        }
        fightTex_ = ch->info.headId >= 0 && ch->info.headId < fightTexData_.size()
            ? &fightTexData_[ch->info.headId] : nullptr;
        fightTexCount_ = ch->info.frame[0];
        fightTexIdx_ = fightTexCount_ * int(ch->direction);
        fightTexCount_ += fightTexIdx_;
        effectTexIdx_ = -ch->info.frameDelay[0];
        fightFrame_ = -ch->info.frameSoundDelay[0];
        return;
    }
    const auto *skillInfo = ::hojy::world::state::gSaveData.skillInfo[actId_];
    if (skillInfo) {
        bool levelup = false;
        effectId_ = skillInfo->effectId;
        auto skillType = skillInfo->skillType;
        stage_ = Acting;
        if ((skillInfo->attackAreaType == 0 || skillInfo->attackAreaType == 3)
            && (cameraX_ != cursorX_ || cameraY_ != cursorY_)) {
            ch->direction = calcDirection(cameraX_, cameraY_, cursorX_, cursorY_);
        }
        fightTex_ = ch->info.headId >= 0 && ch->info.headId < fightTexData_.size()
                    ? &fightTexData_[ch->info.headId] : nullptr;
        fightTexIdx_ = 0;
        for (std::int16_t i = 0; i < skillType; ++i) {
            fightTexIdx_ += 4 * ch->info.frame[i];
        }
        fightTexCount_ = ch->info.frame[skillType];
        fightTexIdx_ += fightTexCount_ * int(ch->direction);
        fightTexCount_ += fightTexIdx_;
        effectTexIdx_ = -ch->info.frameDelay[skillType];
        fightFrame_ = -ch->info.frameSoundDelay[skillType];

        battle::AttackDirection attackDirection = battle::AttackDirection::Up;
        switch (ch->direction) {
        case Map::DirRight: attackDirection = battle::AttackDirection::Right; break;
        case Map::DirDown: attackDirection = battle::AttackDirection::Down; break;
        case Map::DirLeft: attackDirection = battle::AttackDirection::Left; break;
        case Map::DirUp: break;
        }
        const auto attackCells = battle::enumerateAttackCells(
            mapWidth_, mapHeight_, cameraX_, cameraY_, cursorX_, cursorY_,
            skillInfo->attackAreaType, skillInfo->selRange[actLevel_],
            skillInfo->area[actLevel_], attackDirection);
        actionTargets_.clear();
        const auto executedLevel = actLevel_;
        for (const auto &cell: attackCells) {
            makeDamage(ch, cell.x, cell.y, cell.distance);
        }
        ::hojy::world::state::postDamage(
            &ch->info, actIndex_, actLevel_,
            attackTimesLeft_ == 1 ? 3 : 0, skillLevelup_, battleRandom_);
        if (const auto actor = participantIndex(ch)) {
            if (!recordBattleAction(battle::BattleAction{
                    *actor,
                    battle::SkillAction{
                        actIndex_, actId_, executedLevel, actionTargets_,
                    },
                })) {
                return;
            }
        }
        if (skillLevelup_) {
            actLevel_ = std::clamp<std::int16_t>(ch->info.skillLevel[actIndex_] / 100, 0, 9);
        }
    } else {
        actIndex_ = actId_ = -1;
        actLevel_ = 0;
        actItemSlot_ = -1;
        skillLevelup_ = false;
        effectId_ = -1;
        effectTexIdx_ = -1;
        fightTexIdx_ = -1;
        fightTexCount_ = 0;
        fightFrame_ = 0;
        attackTimesLeft_ = 0;
        fightTex_ = nullptr;
        endTurn(ch);
    }
}

void Warfield::makeDamage(Warfield::CharInfo *ch, int x, int y, int distance) {
    auto *info = cellInfo_[y * mapWidth_ + x].charInfo;
    if (!info || info->side == ch->side) { return; }
    auto &enemyInfo = info->info;
    if (enemyInfo.hp > 0) {
        if (const auto target = participantIndex(info)) {
            actionTargets_.push_back(battle::ActionTarget{
                *target, static_cast<std::int16_t>(distance),
            });
        }
    }
    std::int16_t dmg, ps, exp;
    bool dead = false;
    bool wasDead = enemyInfo.hp <= 0;
    if (::hojy::world::state::actDamage(
            &ch->info, &enemyInfo, knowledge_[ch->side], knowledge_[ch->side ^ 1],
            distance, actIndex_, actLevel_, dmg, ps, exp, dead, battleRandom_)) {
        ch->exp += exp;
        if (!wasDead && dead) {
            recalcKnowledge();
        }
        auto *ttf = renderer_->ttf();
        const auto *skillInfo = actId_ > 0 ? ::hojy::world::state::gSaveData.skillInfo[actId_] : nullptr;
        if (skillInfo && battle::isDrainSkill(*skillInfo)) {
            auto txt = fmt::format(L"{:+}", -dmg);
            popupNumbers_.emplace_back(PopupNumber{txt, x, y, 112, 12, 112});
        } else {
            auto txt = fmt::format(L"{:+}", -dmg);
            popupNumbers_.emplace_back(PopupNumber{txt, x, y, 232, 32, 44});
        }
    }
}

}
