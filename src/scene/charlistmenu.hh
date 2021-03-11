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

#include "menu.hh"

#include <string>
#include <vector>
#include <functional>
#include <cstdint>

namespace hojy::scene {

class CharListMenu: public MenuTextList {
public:
    enum ValueType {
        LEVEL,
        HP,
        MAXHP,
        MP,
        MAXMP,
        MEDIC,
        DEPOISON,
        POISONED,
    };

    using MenuTextList::MenuTextList;
    ~CharListMenu() override;

    [[nodiscard]] size_t charCount() const { return charIdList_.size(); }
    [[nodiscard]] std::int16_t charId(size_t idx) const { return idx < charIdList_.size() ? charIdList_[idx] : -1; }
    [[nodiscard]] std::vector<std::int16_t> getSelectedCharIds() const;

    void init(const std::vector<std::wstring> &title, const std::vector<std::int16_t> &charIds,
              const std::vector<ValueType> &valueTypes,
              const std::function<void(std::int16_t)> &okHandler, const std::function<bool()> &cancelHandler = nullptr,
              const std::function<bool(ValueType, std::int16_t)> &filterFunc = nullptr);
    void initWithTeamMembers(const std::vector<std::wstring> &title, const std::vector<ValueType> &valueTypes,
              const std::function<void(std::int16_t)> &okHandler, const std::function<bool()> &cancelHandler = nullptr,
              const std::function<bool(ValueType, std::int16_t)> &filterFunc = nullptr);
    void enableCheckBox(bool b, const std::function<bool(std::int16_t)> &onCheckBoxToggle = nullptr);
    void render() override;

private:
    std::function<bool(std::int16_t)> onCheckBoxToggle2_;
    std::vector<std::int16_t> charIdList_;
    Node *msgBox_ = nullptr;
};

}
