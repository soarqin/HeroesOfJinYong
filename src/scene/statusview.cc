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

#include "statusview.hh"

#include "window.hh"
#include "mem/savedata.hh"
#include "mem/action.hh"
#include "mem/strings.hh"
#include "core/config.hh"
#include "util/math.hh"
#include <fmt/xchar.h>
#include <algorithm>

namespace hojy::scene {

void StatusView::show(const mem::CharacterData *data, bool calcEquip, bool simpleMode) {
    if (!data) { return; }
    data_ = *data;
    simpleMode_ = simpleMode;
    if (calcEquip) { mem::addUpPropFromEquipToChar(&data_); }
}

void StatusView::show(std::int16_t charId) {
    show(mem::gSaveData.charInfo[charId], true);
}

void StatusView::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeyOK: case KeySpace: case KeyCancel:
        delete this;
        break;
    default:
        break;
    }
}

void StatusView::makeCache() {
    auto *ttf = renderer_->ttf();
    auto fontSize = ttf->fontSize();
    auto lineheight = fontSize + TextLineSpacing;
    auto windowBorder = core::config.windowBorder();
    int x0 = windowBorder;
    int x1 = x0 + fontSize * 5 / 2;
    if (simpleMode_) {
        int w = x1 + fontSize * 9 / 2 + windowBorder;
        int h = windowBorder * 2 + lineheight * 8 - TextLineSpacing;
        width_ = w;
        height_ = h;

        cacheBegin();
        renderer_->clear(0, 0, 0, 0);
        renderer_->fillRoundedRect(0, 0, w, h, windowBorder, 64, 64, 64, 208);
        renderer_->drawRoundedRect(0, 0, w, h, windowBorder, 224, 224, 224, 255);
        int y = windowBorder;
        const auto *headTex = gWindow->headTexture(data_.headId);
        if (headTex) {
            auto height = headTex->height();
            std::pair<int, int> scale = util::calcSmallestDivision(lineheight * 4 - TextLineSpacing, height);
            renderer_->renderTexture(headTex, (w - headTex->width() * scale.first / scale.second) / 2, y + lineheight * 4 - TextLineSpacing - height * scale.first / scale.second, scale, true);
        }
        ttf->setColor(236, 236, 236);
        ttf->setAltColor(2, 236, 200, 40);
        ttf->setAltColor(3, 252, 148, 16);
        ttf->setAltColor(4, 196, 8, 16);
        ttf->setAltColor(5, 244, 128, 132);
        ttf->setAltColor(6, 28, 104, 16);
        ttf->setAltColor(7, 96, 176, 64);
        y += lineheight * 4;
        auto name = GETCHARNAME(data_.id);
        ttf->render(name, (w - ttf->stringWidth(name)) / 2, y, true);
        y += lineheight;
        ttf->render(L"\3" + GETTEXT(4), x0, y, true);
        ttf->render(fmt::format(L"\2{:>3}\1/\3{:>3}", data_.stamina, int(data::StaminaMax)), x1, y, true);
        y += lineheight;
        ttf->render(L"\3" + GETTEXT(25), x0, y, true);
        wchar_t c1 = L'\2', c2 = L'\3';
        if (data_.hurt > 66) {
            c1 = L'\4';
        } else if (data_.hurt > 33) {
            c1 = L'\5';
        }
        if (data_.poisoned >= 50) {
            c2 = L'\6';
        } else if (data_.poisoned > 0) {
            c2 = L'\7';
        }
        ttf->render(fmt::format(c1 + std::wstring(L"{:>3}\1/") + c2 + L"{:>3}", data_.hp, data_.maxHp), x1, y, true);
        y += lineheight;
        ttf->render(L"\3" + GETTEXT(26), x0, y, true);
        std::uint8_t r, g, b;
        std::tie(r, g, b) = mem::calcColorForMpType(data_.mpType);
        ttf->setAltColor(16, r, g, b);
        ttf->render(fmt::format(L"\x10{:>3}/{:>3}", data_.mp, data_.maxMp), x1, y, true);
        cacheEnd();
        return;
    }
    int x2 = x1 + fontSize * 9 / 2 + windowBorder;
    int x3 = x2 + fontSize * 9 / 2;
    int x4 = x3 + fontSize * 2 + windowBorder;
    int x5 = x4 + fontSize * 5;
    int w = x5 + fontSize * 3 / 2 + windowBorder;
    bool showPotential = core::config.showPotential();
    int h = windowBorder * 2 + lineheight * 15 - TextLineSpacing;
    width_ = w;
    height_ = h;

    cacheBegin();
    renderer_->clear(0, 0, 0, 0);
    renderer_->fillRoundedRect(0, 0, w, h, windowBorder, 64, 64, 64, 208);
    renderer_->drawRoundedRect(0, 0, w, h, windowBorder, 224, 224, 224, 255);
    int y = windowBorder;
    const auto *headTex = gWindow->headTexture(data_.headId);
    if (headTex) {
        auto height = headTex->height();
        std::pair<int, int> scale = util::calcSmallestDivision(lineheight * 4 - TextLineSpacing, height);
        renderer_->renderTexture(headTex, (x2 - headTex->width() * scale.first / scale.second) / 2,
                                 (y + lineheight * 4 - TextLineSpacing) - height * scale.first / scale.second,
                                 scale, true);
    }
    ttf->setColor(236, 236, 236);
    ttf->setAltColor(2, 236, 200, 40);
    ttf->setAltColor(3, 252, 148, 16);
    ttf->setAltColor(4, 196, 8, 16);
    ttf->setAltColor(5, 244, 128, 132);
    ttf->setAltColor(6, 28, 104, 16);
    ttf->setAltColor(7, 96, 176, 64);
    ttf->render(GETTEXT(8), x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.attack), x3, y, true);
    y += lineheight;
    ttf->render(GETTEXT(10), x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.defence), x3, y, true);
    y += lineheight;
    ttf->render(GETTEXT(9), x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.speed), x3, y, true);
    y += lineheight;
    ttf->render(GETTEXT(11), x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.medic), x3, y, true);
    y += lineheight;
    auto name = GETCHARNAME(data_.id);
    ttf->render(name, (x2 - ttf->stringWidth(name)) / 2, y, true);
    ttf->render(GETTEXT(12), x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.poison), x3, y, true);
    y += lineheight;
    ttf->render(L"\3" + GETTEXT(24), x0, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.level), x1, y, true);
    ttf->render(GETTEXT(13), x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.depoison), x3, y, true);
    y += lineheight;
    ttf->render(L"\3" + GETTEXT(25), x0, y, true);
    wchar_t c1 = L'\2', c2 = L'\3';
    if (data_.hurt > 66) {
        c1 = L'\4';
    } else if (data_.hurt > 33) {
        c1 = L'\5';
    }
    if (data_.poisoned >= 50) {
        c2 = L'\6';
    } else if (data_.poisoned > 0) {
        c2 = L'\7';
    }
    ttf->render(fmt::format(c1 + std::wstring(L"{:>3}\1/") + c2 + L"{:>3}", data_.hp, data_.maxHp), x1, y, true);
    ttf->render(GETTEXT(15), x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.fist), x3, y, true);
    y += lineheight;
    ttf->render(L"\3" + GETTEXT(26), x0, y, true);
    std::uint8_t r, g, b;
    std::tie(r, g, b) = mem::calcColorForMpType(data_.mpType);
    ttf->setAltColor(16, r, g, b);
    ttf->render(fmt::format(L"\x10{:>3}/{:>3}", data_.mp, data_.maxMp), x1, y, true);
    ttf->render(GETTEXT(16), x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.sword), x3, y, true);
    y += lineheight;
    ttf->render(L"\3" + GETTEXT(4), x0, y, true);
    ttf->render(fmt::format(L"\2{:>3}\1/\3{:>3}", data_.stamina, int(data::StaminaMax)), x1, y, true);
    ttf->render(GETTEXT(17), x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.blade), x3, y, true);
    y += lineheight;
    ttf->render(L"\3" + GETTEXT(27), x0, y, true);
    ttf->render(fmt::format(L"\2{:>5}", data_.exp), x1, y, true);
    ttf->render(GETTEXT(18), x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.special), x3, y, true);
    y += lineheight;
    ttf->render(L"\3" + GETTEXT(28), x0, y, true);
    auto exp = mem::getExpForLevelUp(data_.level);
    if (exp) {
        ttf->render(fmt::format(L"\2{:>5}", exp), x1, y, true);
    } else {
        ttf->render(L"\2  =", x1, y, true);
    }
    ttf->render(GETTEXT(19), x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.throwing), x3, y, true);
    if (showPotential) {
        y += lineheight;
        ttf->render(L"\3" + GETTEXT(29), x0, y, true);
        ttf->render(fmt::format(L"\2{:>5}", data_.potential), x1, y, true);
        y += lineheight;
        ttf->render(L"\3" + GETTEXT(20), x0, y, true);
        ttf->render(fmt::format(L"\2{:>5}", data_.knowledge), x1, y, true);
        if (data_.id == 0) {
            y += lineheight;
            ttf->render(L"\3" + GETTEXT(21), x0, y, true);
            ttf->render(fmt::format(L"\2{:>5}", data_.integrity), x1, y, true);
            y += lineheight;
            ttf->render(L"\3" + GETTEXT(114), x0, y, true);
            ttf->render(fmt::format(L"\2{:>5}", data_.reputation), x1, y, true);
        }
    }
    y = windowBorder;
    ttf->render(L"\3" + GETTEXT(30), x4, y, true);
    std::int16_t learningSkillId = -1, learningLevel = 0;
    if (data_.learningItem >= 0) {
        learningSkillId = mem::gSaveData.itemInfo[data_.learningItem]->skillId;
    }
    for (int i = 0; i < data::LearnSkillCount; ++i) {
        y += lineheight;
        if (data_.skillId[i] <= 0) { continue; }
        ttf->render(L'\2' + GETSKILLNAME(data_.skillId[i]), x4, y, true);
        std::int16_t level = std::clamp<std::int16_t>(data_.skillLevel[i] / 100, 0, 9) + 1;
        ttf->render(fmt::format(L"{:>2}", level), x5, y, true);
        if (data_.skillId[i] == learningSkillId) {
            learningLevel = level;
        }
    }
    y = windowBorder + lineheight * 12;
    ttf->render(L"\3" + GETTEXT(31), x2, y, true); ttf->render(L"\3" + GETTEXT(32), x4, y, true);
    y += lineheight;
    if (data_.equip[0] >= 0) {
        ttf->render(L'\2' + GETITEMNAME(data_.equip[0]), x2, y, true);
    }
    if (data_.learningItem >= 0) {
        ttf->render(L'\2' + GETITEMNAME(data_.learningItem), x4, y, true);
    }
    y += lineheight;
    if (data_.equip[1] >= 0) {
        ttf->render(L'\2' + GETITEMNAME(data_.equip[1]), x2, y, true);
    }
    if (data_.learningItem >= 0) {
        std::uint16_t expForItem = mem::getExpForSkillLearn(data_.learningItem, learningLevel - 1, data_.potential);
        ttf->render(L'\2' + fmt::format(L"{:>5}/{:>5}", data_.expForItem, expForItem), x4, y, true);
    }
    cacheEnd();
}

}
