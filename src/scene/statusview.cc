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
#include "core/config.hh"
#include "util/conv.hh"
#include <fmt/format.h>

namespace hojy::scene {

void StatusView::show(const mem::CharacterData *data, bool calcEquip) {
    if (!data) { return; }
    data_ = *data;
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
    int x0 = SubWindowBorder;
    int x1 = x0 + fontSize * 5 / 2;
    int x2 = x1 + fontSize * 9 / 2 + SubWindowBorder;
    int x3 = x2 + fontSize * 9 / 2;
    int x4 = x3 + fontSize * 2 + SubWindowBorder;
    int x5 = x4 + fontSize * 5;
    int w = x5 + fontSize * 3 / 2 + SubWindowBorder;
    bool showPotential = core::config.showPotential();
    int h = SubWindowBorder * 2 + lineheight * 15 - TextLineSpacing;
    width_ = w;
    height_ = h;

    cacheBegin();
    renderer_->clear(0, 0, 0, 0);
    renderer_->fillRoundedRect(0, 0, w, h, RoundedRectRad, 64, 64, 64, 208);
    renderer_->drawRoundedRect(0, 0, w, h, RoundedRectRad, 224, 224, 224, 255);
    int y = SubWindowBorder;
    const auto *headTex = gWindow->headTexture(data_.headId);
    if (headTex) {
        auto height = float(headTex->height());
        float scale = float(lineheight * 4 - TextLineSpacing) / height;
        renderer_->renderTexture(headTex, (float(x2) - float(headTex->width()) * scale) / 2.f, float(y + lineheight * 4 - TextLineSpacing) - height * scale, scale, true);
    }
    ttf->setColor(236, 236, 236);
    ttf->setAltColor(2, 236, 200, 40);
    ttf->setAltColor(3, 252, 148, 16);
    ttf->setAltColor(4, 196, 8, 16);
    ttf->setAltColor(5, 244, 128, 132);
    ttf->setAltColor(6, 28, 104, 16);
    ttf->setAltColor(7, 96, 176, 64);
    ttf->render(L"攻擊力", x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.attack), x3, y, true);
    y += lineheight;
    ttf->render(L"防禦力", x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.defence), x3, y, true);
    y += lineheight;
    ttf->render(L"輕功", x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.speed), x3, y, true);
    y += lineheight;
    ttf->render(L"醫療能力", x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.medic), x3, y, true);
    y += lineheight;
    auto name = util::big5Conv.toUnicode(data_.name);
    ttf->render(name, (x2 - ttf->stringWidth(name)) / 2, y, true);
    ttf->render(L"用毒能力", x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.poison), x3, y, true);
    y += lineheight;
    ttf->render(L"\3等級", x0, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.level), x1, y, true);
    ttf->render(L"解毒能力", x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.depoison), x3, y, true);
    y += lineheight;
    ttf->render(L"\3生命", x0, y, true);
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
    ttf->render(L"拳掌功夫", x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.fist), x3, y, true);
    y += lineheight;
    ttf->render(L"\3內力", x0, y, true);
    std::uint8_t r, g, b;
    std::tie(r, g, b) = mem::calcColorForMpType(data_.mpType);
    ttf->setAltColor(16, r, g, b);
    ttf->render(fmt::format(L"\x10{:>3}/{:>3}", data_.mp, data_.maxMp), x1, y, true);
    ttf->render(L"御劍能力", x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.sword), x3, y, true);
    y += lineheight;
    ttf->render(L"\3體力", x0, y, true);
    ttf->render(fmt::format(L"\2{:>3}\1/\3{:>3}", data_.stamina, data::StaminaMax), x1, y, true);
    ttf->render(L"耍刀技巧", x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.blade), x3, y, true);
    y += lineheight;
    ttf->render(L"\3經驗", x0, y, true);
    ttf->render(fmt::format(L"\2{:>5}", data_.exp), x1, y, true);
    ttf->render(L"特殊兵器", x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.special), x3, y, true);
    y += lineheight;
    ttf->render(L"\3升級", x0, y, true);
    auto exp = mem::getExpForLevelUp(data_.level);
    if (exp) {
        ttf->render(fmt::format(L"\2{:>5}", exp), x1, y, true);
    } else {
        ttf->render(L"\2  =", x1, y, true);
    }
    ttf->render(L"暗器技巧", x2, y, true);
    ttf->render(fmt::format(L"\2{:>3}", data_.throwing), x3, y, true);
    if (showPotential) {
        y += lineheight;
        ttf->render(L"\3資質", x0, y, true);
        ttf->render(fmt::format(L"\2{:>5}", data_.potential), x1, y, true);
    }
    y = SubWindowBorder;
    ttf->render(L"\3所會功夫", x4, y, true);
    std::int16_t learningSkillId = -1, learningLevel = 0;
    if (data_.learningItem >= 0) {
        learningSkillId = mem::gSaveData.itemInfo[data_.learningItem]->skillId;
    }
    for (int i = 0; i < data::LearnSkillCount; ++i) {
        y += lineheight;
        if (data_.skillId[i] <= 0) { continue; }
        ttf->render(L'\2' + util::big5Conv.toUnicode(mem::gSaveData.skillInfo[data_.skillId[i]]->name), x4, y, true);
        std::int16_t level = std::clamp<int16_t>(data_.skillLevel[i] / 100, 0, 9) + 1;
        ttf->render(fmt::format(L"{:>2}", level), x5, y, true);
        if (data_.skillId[i] == learningSkillId) {
            learningLevel = level;
        }
    }
    y = SubWindowBorder + lineheight * 12;
    ttf->render(L"\3裝備物品", x0, y, true); ttf->render(L"\3修練物品", x2, y, true);
    y += lineheight;
    if (data_.equip[0] >= 0) {
        ttf->render(L'\2' + util::big5Conv.toUnicode(mem::gSaveData.itemInfo[data_.equip[0]]->name), x0, y, true);
    }
    if (data_.learningItem >= 0) {
        ttf->render(L'\2' + util::big5Conv.toUnicode(mem::gSaveData.itemInfo[data_.learningItem]->name), x2, y, true);
    }
    y += lineheight;
    if (data_.equip[1] >= 0) {
        ttf->render(L'\2' + util::big5Conv.toUnicode(mem::gSaveData.itemInfo[data_.equip[1]]->name), x0, y, true);
    }
    if (data_.learningItem >= 0) {
        std::uint16_t expForItem = mem::getExpForSkillLearn(data_.learningItem, learningLevel, data_.potential);
        ttf->render(L'\2' + fmt::format(L"{:>5}/{:>5}", data_.expForItem, expForItem), x2, y, true);
    }
    cacheEnd();
}

}
