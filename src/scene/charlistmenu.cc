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

#include "charlistmenu.hh"

#include "messagebox.hh"
#include "window.hh"
#include "mem/strings.hh"
#include "mem/savedata.hh"

#include <fmt/format.h>

namespace hojy::scene {

std::int16_t getValueFromType(const mem::CharacterData *info, CharListMenu::ValueType t) {
    switch (t) {
    case CharListMenu::LEVEL:
        return info->level;
    case CharListMenu::HP:
        return info->hp;
    case CharListMenu::MAXHP:
        return info->maxHp;
    case CharListMenu::MP:
        return info->mp;
    case CharListMenu::MAXMP:
        return info->maxMp;
    case CharListMenu::MEDIC:
        return info->medic;
    case CharListMenu::DEPOISON:
        return info->depoison;
    case CharListMenu::POISONED:
        return info->poisoned;
    default:
        return 0;
    }
}

void getNameFromTypeList(const std::vector<CharListMenu::ValueType> &valueTypes, std::wstring &result) {
    result.clear();
    for (auto &t: valueTypes) {
        if (!result.empty()) { result += L'|'; }
        switch (t) {
        case CharListMenu::LEVEL:
            result.append(GETTEXT(24));
            break;
        case CharListMenu::HP:
            result.append(GETTEXT(1));
            break;
        case CharListMenu::MAXHP:
            result.append(GETTEXT(2));
            break;
        case CharListMenu::MP:
            result.append(GETTEXT(6));
            break;
        case CharListMenu::MAXMP:
            result.append(GETTEXT(7));
            break;
        case CharListMenu::MEDIC:
            result.append(GETTEXT(11));
            break;
        case CharListMenu::DEPOISON:
            result.append(GETTEXT(13));
            break;
        case CharListMenu::POISONED:
            result.append(GETTEXT(3));
            break;
        default:
            break;
        }
    }
}

void listNamesFromTypeList(const std::vector<std::int16_t> &charIdList,
                           const std::vector<CharListMenu::ValueType> &valueTypes,
                           std::vector<std::wstring> &names, std::vector<std::wstring> &values) {
    names.clear();
    for (auto id: charIdList) {
        auto *charInfo = mem::gSaveData.charInfo[std::abs(id)];
        if (!charInfo) { continue; }
        if (id < 0) {
            names.emplace_back(L'\x0F' + GETCHARNAME(-id));
        } else {
            names.emplace_back(GETCHARNAME(id));
        }
        if (valueTypes.empty()) { continue; }
        std::wstring value;
        for (auto t: valueTypes) {
            if (!value.empty()) { value += L'|'; }
            switch (t) {
            case CharListMenu::LEVEL:
                value.append(fmt::format(L"{:>3}", charInfo->level));
                break;
            case CharListMenu::HP:
                value.append(fmt::format(L"{:>3}/{:>3}", charInfo->hp, charInfo->maxHp));
                break;
            case CharListMenu::MAXHP:
                value.append(fmt::format(L"{:>3}", charInfo->maxHp));
                break;
            case CharListMenu::MP:
                value.append(fmt::format(L"{:>3}/{:>3}", charInfo->mp, charInfo->maxMp));
                break;
            case CharListMenu::MAXMP:
                value.append(fmt::format(L"{:>3}", charInfo->maxMp));
                break;
            case CharListMenu::MEDIC:
                value.append(fmt::format(L"{:>3}", charInfo->medic));
                break;
            case CharListMenu::DEPOISON:
                value.append(fmt::format(L"{:>3}", charInfo->depoison));
                break;
            case CharListMenu::POISONED:
                value.append(fmt::format(L"{:>3}", charInfo->poisoned));
                break;
            default:
                break;
            }
        }
        values.emplace_back(value);
    }
}

CharListMenu::~CharListMenu() {
    delete msgBox_;
}

std::vector<std::int16_t> CharListMenu::getSelectedCharIds() const {
    std::vector<std::int16_t> res;
    if (checkbox_) {
        for (size_t i = 0; i < selected_.size(); ++i) {
            if (selected_[i]) {
                res.emplace_back(std::abs(charIdList_[i]));
            }
        }
    } else {
        if (currIndex_ >= 0) {
            res.emplace_back(std::abs(charIdList_[currIndex_]));
        }
    }
    return res;
}

void CharListMenu::init(const std::vector<std::wstring> &title, const std::vector<std::int16_t> &charIds,
                        const std::vector<ValueType> &valueTypes,
                        const std::function<void(std::int16_t)> &okHandler, const std::function<bool()> &cancelHandler,
                        const std::function<bool(ValueType, std::int16_t)> &filterFunc) {
    if (!valueTypes.empty() && filterFunc) {
        charIdList_.clear();
        for (auto id: charIds) {
            auto *charInfo = mem::gSaveData.charInfo[id];
            if (!charInfo) { continue; }
            bool add = true;
            for (auto t: valueTypes) {
                if (!filterFunc(t, getValueFromType(charInfo, t))) {
                    add = false;
                    break;
                }
            }
            if (add) {
                charIdList_.emplace_back(id);
            }
        }
    } else {
        charIdList_ = charIds;
    }
    MessageBox *msgBox = nullptr;
    if (!title.empty()) {
        msgBox = new MessageBox(renderer_, x_, y_, gWindow->width() - x_, gWindow->height() - y_);
        msgBox_ = msgBox;
        msgBox->popup(title, MessageBox::Normal, MessageBox::TopLeft);
        msgBox->forceUpdate();
        y_ += msgBox->height() + 10;
    }
    std::vector<std::wstring> names, values;
    listNamesFromTypeList(charIdList_, valueTypes, names, values);
    if (!valueTypes.empty()) {
        std::wstring subTitle;
        getNameFromTypeList(valueTypes, subTitle);
        setTitle(subTitle);
    }
    auto *ttf = renderer_->ttf();
    ttf->setAltColor(15, 252, 100, 12);
    popup(names, values);
    setHandler([this, okHandler]() {
        if (!okHandler) { return; }
        if (checkbox_) {
            okHandler(0);
            return;
        }
        if (currIndex_ >= 0) {
            okHandler(std::abs(charIdList_[currIndex_]));
        }
    }, [this, cancelHandler]()->bool {
        if (cancelHandler) { return cancelHandler(); }
        return true;
    });
}

void CharListMenu::initWithTeamMembers(const std::vector<std::wstring> &title, const std::vector<ValueType> &valueTypes,
                                       const std::function<void(std::int16_t)> &okHandler,
                                       const std::function<bool()> &cancelHandler,
                                       const std::function<bool(ValueType, std::int16_t)> &filterFunc) {
    std::vector<std::int16_t> charIds;
    for (auto id: mem::gSaveData.baseInfo->members) {
        if (id >= 0) {
            charIds.emplace_back(id);
        }
    }
    init(title, charIds, valueTypes, okHandler, cancelHandler, filterFunc);
}

void CharListMenu::enableCheckBox(bool b, const std::function<bool(std::int16_t)> &onCheckBoxToggle) {
    if (!b) {
        Menu::enableCheckBox(false);
    } else {
        onCheckBoxToggle2_ = onCheckBoxToggle;
        Menu::enableCheckBox(true, [this](int index)->bool {
            if (index < 0) { return false; }
            return onCheckBoxToggle2_(std::abs(charIdList_[currIndex_]));
        });
    }
}

void CharListMenu::makeCenter(int w, int h, int x, int y) {
    NodeWithCache::makeCenter(w, h, x, y);
    if (msgBox_) {
        msgBox_->setPosition(x_, y_);
        y_ += msgBox_->height() + 10;
    }
}

void CharListMenu::render() {
    msgBox_->render();
    NodeWithCache::render();
}

}
