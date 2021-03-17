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

#include "map.hh"
#include "mem/character.hh"
#include <vector>
#include <map>
#include <set>

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
        mem::CharacterData info;
        std::uint16_t exp;
        std::int16_t steps;
        std::int16_t attack, defence;
    };
    struct CellInfo {
        const Texture *earth = nullptr, *building = nullptr, *effect = nullptr;
        bool blocked = false;
        CharInfo *charInfo = nullptr;
        std::uint8_t insideMovingArea = 0;
    };
    struct SelectableCell {
        int x, y, moves, ranges;
        SelectableCell *moveParent, *rangeParent;
    };
    struct PopupNumber {
        std::wstring str;
        int x, y;
        int offsetX;
        std::uint8_t r, g, b;
    };
public:
    Warfield(Renderer *renderer, int x, int y, int width, int height, float scale);
    ~Warfield() override;

    void cleanup();
    bool load(std::int16_t warId);
    inline void setGetExpOnLose(bool b) { getExpOnLose_ = b; }
    inline void setDeadOnLose(bool b) { deadOnLose_ = b; }
    bool getDefaultChars(std::set<std::int16_t> &chars) const;
    void putChars(const std::vector<std::int16_t> &chars);

    void render() override;
    void handleKeyInput(Key key) override;

protected:
    void frameUpdate() override;

    void nextAction();
    void autoAction();
    void recalcKnowledge();
    void playerMenu();
    void maskSelectableArea(int steps, int ranges, bool zoecheck = false);
    void unmaskArea();
    void getSelectableArea(CharInfo *ch, std::map<std::pair<int, int>, SelectableCell> &selCells, int steps, int ranges, bool zoecheck = false);
    bool tryUseSkill(int index);
    void startActAction();
    void makeDamage(CharInfo *ch, int x, int y, int distance);
    void doRest();
    void endTurn();
    void endWar();
    void popupFinishMessages(std::vector<std::pair<int, std::wstring>> messages, int index);

private:
    std::int16_t warId_ = -1;
    bool getExpOnLose_ = false;
    bool deadOnLose_ = false;
    std::vector<CellInfo> cellInfo_;
    std::set<std::int16_t> warMapLoaded_;

    std::vector<CharInfo> chars_;
    std::vector<CharInfo*> charQueue_;
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
    int effectId_ = -1, effectTexIdx_ = -1, fightTexIdx_ = -1, fightTexCount_ = 0, fightFrame_ = 0;
    int attackTimesLeft_ = 0;
    const TextureMgr *fightTexMgr_ = nullptr;
    std::vector<PopupNumber> popupNumbers_;
    std::function<void()> pendingAutoAction_;

    Texture *maskTex_ = nullptr;
    std::vector<TextureMgr> fightTextures_;
};

}
