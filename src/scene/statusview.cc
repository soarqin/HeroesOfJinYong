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

#include "core/config.hh"
#include "texture.hh"
#include "util/math.hh"
#include <fmt/xchar.h>
#include <algorithm>
#include <utility>

namespace hojy::scene {

void StatusView::show(CharacterStatusSnapshot snapshot) {
    simpleMode_ = snapshot.simpleMode;
    data_ = std::move(snapshot);
    requestPresentationRefresh();
}

void StatusView::setBattleAnchor(bool left, int width, int height, int border) noexcept {
    battleAnchorEnabled_ = true;
    battleAnchorLeft_ = left;
    battleAreaWidth_ = width;
    battleAreaHeight_ = height;
    battleAnchorBorder_ = border;
    setInputEnabled(false);
}

void StatusView::prepareRender() {
    NodeWithCache::prepareRender();
    if (!battleAnchorEnabled_ || !renderCacheReady()) { return; }
    setPosition(battleAnchorLeft_ ? battleAnchorBorder_ * 4
                                  : battleAreaWidth_ - battleAnchorBorder_ * 4 - width_,
                battleAreaHeight_ * 2 / 5 - height_ / 2);
}

void StatusView::consumeKeyIntent(Node::Key key) {
    pendingInput_ = key;
}

void StatusView::applyInputLogic() {
    const auto key = pendingInput_;
    pendingInput_ = KeyNone;
    switch (key) {
    case KeyOK: case KeySpace: case KeyCancel:
        requestPresentationCleanup();
        break;
    default:
        break;
    }
}

bool StatusView::prepareTextResources() {
    auto *ttf = renderer_->ttf();
    bool ready = ttf->prepareText(L"0123456789 +-=/()=");
    ready = ttf->prepareText(data_.name) && ready;
    for (const auto &label: data_.labels) {
        ready = ttf->prepareText(label) && ready;
    }
    for (int i = 0; i < ::hojy::content::LearnSkillCount; ++i) {
        ready = ttf->prepareText(data_.skillNames[static_cast<std::size_t>(i)]) && ready;
    }
    for (const auto &name: data_.equipNames) {
        ready = ttf->prepareText(name) && ready;
    }
    ready = ttf->prepareText(data_.learningItemName) && ready;
    return ready;
}

void StatusView::ensureLayout() {
    auto fontSize = renderer_->fontSize();
    auto lineheight = fontSize + TextLineSpacing;
    auto windowBorder = core::config.windowBorder();
    int x0 = windowBorder;
    int x1 = x0 + fontSize * 5 / 2;
    if (simpleMode_) {
        width_ = x1 + fontSize * 9 / 2 + windowBorder;
        height_ = windowBorder * 2 + lineheight * 8 - TextLineSpacing;
        return;
    }
    int x2 = x1 + fontSize * 9 / 2 + windowBorder;
    int x3 = x2 + fontSize * 9 / 2;
    int x4 = x3 + fontSize * 2 + windowBorder;
    int x5 = x4 + fontSize * 5;
    width_ = x5 + fontSize * 3 / 2 + windowBorder;
    height_ = windowBorder * 2 + lineheight * 15 - TextLineSpacing;
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
        const auto *headTex = headTextureProvider_ ? headTextureProvider_(data_.headId) : nullptr;
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
        const auto &name = data_.name;
        ttf->renderPrepared(name, (w - ttf->preparedStringWidth(name)) / 2, y, true);
        y += lineheight;
        ttf->renderPrepared(L"\3" + data_.text(4), x0, y, true);
        ttf->renderPrepared(fmt::format(L"\2{:>3}\1/\3{:>3}", data_.stamina, int(::hojy::content::StaminaMax)), x1, y, true);
        y += lineheight;
        ttf->renderPrepared(L"\3" + data_.text(25), x0, y, true);
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
        ttf->renderPrepared(fmt::format(c1 + std::wstring(L"{:>3}\1/") + c2 + L"{:>3}", data_.hp, data_.maxHp), x1, y, true);
        y += lineheight;
        ttf->renderPrepared(L"\3" + data_.text(26), x0, y, true);
        ttf->setAltColor(16, data_.mpColorR, data_.mpColorG, data_.mpColorB);
        ttf->renderPrepared(fmt::format(L"\x10{:>3}/{:>3}", data_.mp, data_.maxMp), x1, y, true);
        cacheEnd();
        return;
    }
    int x2 = x1 + fontSize * 9 / 2 + windowBorder;
    int x3 = x2 + fontSize * 9 / 2;
    int x4 = x3 + fontSize * 2 + windowBorder;
    int x5 = x4 + fontSize * 5;
    int w = x5 + fontSize * 3 / 2 + windowBorder;
    const bool showPotential = data_.showPotential;
    int h = windowBorder * 2 + lineheight * 15 - TextLineSpacing;
    width_ = w;
    height_ = h;

    cacheBegin();
    renderer_->clear(0, 0, 0, 0);
    renderer_->fillRoundedRect(0, 0, w, h, windowBorder, 64, 64, 64, 208);
    renderer_->drawRoundedRect(0, 0, w, h, windowBorder, 224, 224, 224, 255);
    int y = windowBorder;
    const auto *headTex = headTextureProvider_ ? headTextureProvider_(data_.headId) : nullptr;
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
    ttf->renderPrepared(data_.text(8), x2, y, true);
    ttf->renderPrepared(fmt::format(L"\2{:>3}", data_.attack), x3, y, true);
    y += lineheight;
    ttf->renderPrepared(data_.text(10), x2, y, true);
    ttf->renderPrepared(fmt::format(L"\2{:>3}", data_.defence), x3, y, true);
    y += lineheight;
    ttf->renderPrepared(data_.text(9), x2, y, true);
    ttf->renderPrepared(fmt::format(L"\2{:>3}", data_.speed), x3, y, true);
    y += lineheight;
    ttf->renderPrepared(data_.text(11), x2, y, true);
    ttf->renderPrepared(fmt::format(L"\2{:>3}", data_.medic), x3, y, true);
    y += lineheight;
    const auto &name = data_.name;
    ttf->renderPrepared(name, (x2 - ttf->preparedStringWidth(name)) / 2, y, true);
    ttf->renderPrepared(data_.text(12), x2, y, true);
    ttf->renderPrepared(fmt::format(L"\2{:>3}", data_.poison), x3, y, true);
    y += lineheight;
    ttf->renderPrepared(L"\3" + data_.text(24), x0, y, true);
    ttf->renderPrepared(fmt::format(L"\2{:>3}", data_.level), x1, y, true);
    ttf->renderPrepared(data_.text(13), x2, y, true);
    ttf->renderPrepared(fmt::format(L"\2{:>3}", data_.depoison), x3, y, true);
    y += lineheight;
    ttf->renderPrepared(L"\3" + data_.text(25), x0, y, true);
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
    ttf->renderPrepared(fmt::format(c1 + std::wstring(L"{:>3}\1/") + c2 + L"{:>3}", data_.hp, data_.maxHp), x1, y, true);
    ttf->renderPrepared(data_.text(15), x2, y, true);
    ttf->renderPrepared(fmt::format(L"\2{:>3}", data_.fist), x3, y, true);
    y += lineheight;
    ttf->renderPrepared(L"\3" + data_.text(26), x0, y, true);
    ttf->setAltColor(16, data_.mpColorR, data_.mpColorG, data_.mpColorB);
    ttf->renderPrepared(fmt::format(L"\x10{:>3}/{:>3}", data_.mp, data_.maxMp), x1, y, true);
    ttf->renderPrepared(data_.text(16), x2, y, true);
    ttf->renderPrepared(fmt::format(L"\2{:>3}", data_.sword), x3, y, true);
    y += lineheight;
    ttf->renderPrepared(L"\3" + data_.text(4), x0, y, true);
    ttf->renderPrepared(fmt::format(L"\2{:>3}\1/\3{:>3}", data_.stamina, int(::hojy::content::StaminaMax)), x1, y, true);
    ttf->renderPrepared(data_.text(17), x2, y, true);
    ttf->renderPrepared(fmt::format(L"\2{:>3}", data_.blade), x3, y, true);
    y += lineheight;
    ttf->renderPrepared(L"\3" + data_.text(27), x0, y, true);
    ttf->renderPrepared(fmt::format(L"\2{:>5}", data_.exp), x1, y, true);
    ttf->renderPrepared(data_.text(18), x2, y, true);
    ttf->renderPrepared(fmt::format(L"\2{:>3}", data_.special), x3, y, true);
    y += lineheight;
    ttf->renderPrepared(L"\3" + data_.text(28), x0, y, true);
    const auto exp = data_.expForLevelUp;
    if (exp) {
        ttf->renderPrepared(fmt::format(L"\2{:>5}", exp), x1, y, true);
    } else {
        ttf->renderPrepared(L"\2  =", x1, y, true);
    }
    ttf->renderPrepared(data_.text(19), x2, y, true);
    ttf->renderPrepared(fmt::format(L"\2{:>3}", data_.throwing), x3, y, true);
    if (showPotential) {
        y += lineheight;
        ttf->renderPrepared(L"\3" + data_.text(29), x0, y, true);
        ttf->renderPrepared(fmt::format(L"\2{:>5}", data_.potential), x1, y, true);
        y += lineheight;
        ttf->renderPrepared(L"\3" + data_.text(20), x0, y, true);
        ttf->renderPrepared(fmt::format(L"\2{:>5}", data_.knowledge), x1, y, true);
        if (data_.id == 0) {
            y += lineheight;
            ttf->renderPrepared(L"\3" + data_.text(21), x0, y, true);
            ttf->renderPrepared(fmt::format(L"\2{:>5}", data_.integrity), x1, y, true);
            y += lineheight;
            ttf->renderPrepared(L"\3" + data_.text(114), x0, y, true);
            ttf->renderPrepared(fmt::format(L"\2{:>5}", data_.reputation), x1, y, true);
        }
    }
    y = windowBorder;
    ttf->renderPrepared(L"\3" + data_.text(30), x4, y, true);
    const auto learningSkillId = data_.learningSkillId;
    const auto learningLevel = data_.learningLevel;
    for (int i = 0; i < ::hojy::content::LearnSkillCount; ++i) {
        y += lineheight;
        if (data_.skillId[i] <= 0) { continue; }
        ttf->renderPrepared(L'\2' + data_.skillNames[static_cast<std::size_t>(i)], x4, y, true);
        std::int16_t level = std::clamp<std::int16_t>(data_.skillLevel[i] / 100, 0, 9) + 1;
        ttf->renderPrepared(fmt::format(L"{:>2}", level), x5, y, true);
    }
    y = windowBorder + lineheight * 12;
    ttf->renderPrepared(L"\3" + data_.text(31), x2, y, true); ttf->renderPrepared(L"\3" + data_.text(32), x4, y, true);
    y += lineheight;
    if (data_.equip[0] >= 0) {
        ttf->renderPrepared(L'\2' + data_.equipNames[0], x2, y, true);
    }
    if (data_.learningItem >= 0) {
        ttf->renderPrepared(L'\2' + data_.learningItemName, x4, y, true);
    }
    y += lineheight;
    if (data_.equip[1] >= 0) {
        ttf->renderPrepared(L'\2' + data_.equipNames[1], x2, y, true);
    }
    if (data_.learningItem >= 0) {
        ttf->renderPrepared(L'\2' + fmt::format(L"{:>5}/{:>5}", data_.expForItem,
                                                data_.expForSkillLearn), x4, y, true);
    }
    cacheEnd();
}

}
