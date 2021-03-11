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
#include "mem/savedata.hh"
#include "util/conv.hh"

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
            result.append(L"等級");
            break;
        case CharListMenu::HP:
            result.append(L"生命點數");
            break;
        case CharListMenu::MAXHP:
            result.append(L"最大生命點數");
            break;
        case CharListMenu::MP:
            result.append(L"内力點數");
            break;
        case CharListMenu::MAXMP:
            result.append(L"最大内力點數");
            break;
        case CharListMenu::MEDIC:
            result.append(L"醫療能力");
            break;
        case CharListMenu::DEPOISON:
            result.append(L"解毒能力");
            break;
        case CharListMenu::POISONED:
            result.append(L"中毒程度");
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
        auto *charInfo = mem::gSaveData.charInfo[id];
        if (!charInfo) { continue; }
        names.emplace_back(util::big5Conv.toUnicode(charInfo->name));
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

void CharListMenu::init(const std::vector<std::wstring> &title, const std::vector<std::int16_t> &charIds,
                        const std::vector<ValueType> &valueTypes,
                        const std::function<void(std::int16_t)> &okHandler, const std::function<void()> &cancelHandler,
                        const std::function<bool(ValueType, std::int16_t)> &filterFunc) {
    std::vector<std::int16_t> charIdList;
    if (!valueTypes.empty() && filterFunc) {
        charIdList.clear();
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
                charIdList.emplace_back(id);
            }
        }
    } else {
        charIdList = charIds;
    }
    MessageBox *msgBox = nullptr;
    if (!title.empty()) {
        msgBox = new MessageBox(renderer_, x_, y_, parent_->width() - x_, parent_->height() - y_);
        parent_->addAtFront(msgBox);
        msgBox->popup(title, MessageBox::Normal, MessageBox::TopLeft);
        msgBox->forceUpdate();
        y_ += msgBox->height() + 10;
    }
    std::vector<std::wstring> names, values;
    listNamesFromTypeList(charIdList, valueTypes, names, values);
    if (!valueTypes.empty()) {
        std::wstring subTitle;
        getNameFromTypeList(valueTypes, subTitle);
        setTitle(subTitle);
    }
    popup(names, values);
    setHandler([charIdList, okHandler](int index) {
        if (index >= 0) {
            if (okHandler) { okHandler(charIdList[index]); }
        }
    }, [this, cancelHandler, msgBox]() {
        auto *box = msgBox;
        auto fn = cancelHandler;
        delete this;
        delete box;
        if (fn) { fn(); }
    });
}

void CharListMenu::initWithTeamMembers(const std::vector<std::wstring> &title, const std::vector<ValueType> &valueTypes,
                                       const std::function<void(std::int16_t)> &okHandler,
                                       const std::function<void()> &cancelHandler,
                                       const std::function<bool(ValueType, std::int16_t)> &filterFunc) {
    std::vector<std::int16_t> charIds;
    for (auto id: mem::gSaveData.baseInfo->members) {
        if (id >= 0) {
            charIds.emplace_back(id);
        }
    }
    init(title, charIds, valueTypes, okHandler, cancelHandler, filterFunc);
}

}
