/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "battle/ai.hh"
#include "battle/ai_policy.hh"
#include "battle/engine.hh"
#include "battle/game_random.hh"
#include "battle/movement.hh"
#include "map.hh"
#include "world/bag.hh"
#include "world/character.hh"
#include <functional>
#include <memory>
#include <map>
#include <optional>
#include <set>
#include <vector>

namespace hojy::scene {

class Warfield: public Map {
    enum {
        FightTextureListCount = 110,
    };
    enum Stage {
        Idle,
        PlayerMenu,
        MoveSelecting,
        AttackSelecting,
        Moving,
        Acting,
        PoppingUp,
        Finished,
    };
    struct CharInfo {
        std::uint8_t side; /* 0-self 1-enemy */
        std::int16_t id;
        std::int16_t texId;
        std::int16_t x, y;
        Direction direction;
        ::hojy::world::state::CharacterData info;
        std::uint16_t exp;
        std::int16_t steps;
        std::int16_t initialSteps;
        std::int16_t attack, defence;
        battle::AiStats aiEntryStats;
        battle::AiStats aiEquipmentBonusStats;
        /* 8/9 are persistent AI request markers (medic/depoison), not UI-only codes. */
        std::int16_t actionCode = 0;
        std::int16_t persistentEntryMaxMp = 0;
        std::int16_t battleEntryMaxMp = 0;
    };
    struct CellInfo {
        std::int16_t earthId = 0, buildingId = 0;
        bool blocked = false;
        CharInfo *charInfo = nullptr;
        std::uint8_t insideMovingArea = 0;
    };
    using SelectableCell = battle::SelectableCell;
    struct PopupNumber {
        std::wstring str;
        int x, y;
        std::uint8_t r, g, b;
    };
public:
    Warfield(Renderer *renderer, int x, int y, int width, int height, std::pair<int, int> scale);
    ~Warfield() override;

    void cleanup();
    bool load(std::int16_t warId);
    inline void setGetExpOnLose(bool b) { getExpOnLose_ = b; }
    inline void setDeadOnLose(bool b) { deadOnLose_ = b; }
    bool getDefaultChars(std::set<std::int16_t> &chars) const;
    bool putChars(const std::vector<std::int16_t> &chars);

    void render() override;
    void handleKeyInput(Key key) override;

protected:
    void frameUpdate() override;

    void nextAction();
    void autoAction();
    void autoActionSkill(CharInfo *ch, int actorIndex,
                         const battle::AiStats &actorAiStats,
                         const battle::AiResourceState &resourceState,
                         const std::vector<battle::AiAllyState> &allies,
                         const battle::AiPowerSummary &allyPower,
                         bool resumeAutoAttack, bool requestSupport,
                         bool supportWithoutPosition,
                         battle::AiResourceAction resourceAction,
                         battle::RandomSource &resourceRandom);
    void recalcKnowledge();
    void playerMenu();
    void maskSelectableArea(int steps, int ranges, bool zoecheck = false);
    void unmaskArea();
    void getSelectableArea(CharInfo *ch, std::map<std::pair<int, int>, SelectableCell> &selCells, int steps, int ranges, bool zoecheck = false);
    bool tryUseSkill(int index);
    void startActAction();
    void makeDamage(CharInfo *ch, int x, int y, int distance);
    void doRest(CharInfo *expectedActor = nullptr);
    void endTurn(CharInfo *expectedActor = nullptr);
    bool checkWarEnd();
    void endWar();
    void clearActionState(bool clearPopupNumbers);
    void popupFinishMessages(std::vector<std::pair<int, std::wstring>> messages, int index);
    void syncBattleParticipantsToWorking() noexcept;
    void syncBattleParticipantsFromWorking() noexcept;
    void discardBattleSession() noexcept;
    void commitBattleBag() noexcept;
    [[nodiscard]] std::optional<battle::ParticipantId> participantIndex(
        const CharInfo *character) const noexcept;
    [[nodiscard]] battle::InventorySnapshot battleInventorySnapshot() const;
    bool recordBattleAction(const battle::BattleAction &action);

private:
    std::int16_t warId_ = -1;
    bool getExpOnLose_ = false;
    bool deadOnLose_ = false;
    std::vector<CellInfo> cellInfo_;
    std::set<std::int16_t> warMapLoaded_;

    std::vector<CharInfo> chars_;
    std::vector<std::unique_ptr<battle::BattleParticipant>> battleParticipants_;
    battle::GameRandom battleGameRandom_;
    battle::RecordingRandom battleRandom_;
    battle::BattleEngine battleEngine_;
    ::hojy::world::state::Bag battleBag_;
    bool battleBagActive_ = false;
    std::vector<CharInfo*> turnOrder_;
    std::vector<CharInfo*> charQueue_;
    CharInfo *currentActor_ = nullptr;
    std::uint32_t round_ = 0;
    Stage stage_ = Idle;
    int lastMenuIndex_ = 0;
    std::uint16_t knowledge_[2] = {0, 0};
    int cursorX_ = 0, cursorY_ = 0;
    bool autoControl_ = false;
    bool won_ = false;
    bool skillLevelup_ = false;
    std::map<std::pair<int, int>, SelectableCell> selCells_;
    std::vector<std::pair<int, int>> movingPath_;
    /* -3poison -2depoison -1medic 0~skillId */
    std::int16_t actIndex_ = -1, actId_ = -1, actLevel_ = 0;
    std::int16_t actItemSlot_ = -1;
    std::vector<battle::ActionTarget> actionTargets_;
    int effectId_ = -1, effectTexIdx_ = -1, fightTexIdx_ = -1, fightTexCount_ = 0, fightFrame_ = 0;
    int attackTimesLeft_ = 0;
    const std::vector<std::string> *fightTex_ = nullptr;
    std::vector<PopupNumber> popupNumbers_;
    std::function<void()> pendingAutoAction_;
    bool resumeAutoAttack_ = false;
    Node *statusPanel_ = nullptr;
    Texture *drawingTerrainTex2_ = nullptr;
    std::vector<std::vector<std::string>> fightTexData_;
};

}
