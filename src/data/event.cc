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

#include "event.hh"

#include "grpdata.hh"
#include "core/config.hh"
#include "util/conv.hh"
#include <cstring>

namespace hojy::data {

Event gEvent;

void Event::loadEvent(const std::string &name) {
    GrpData::DataSet dset;
    if (!GrpData::loadData(name, dset)) { return; }
    auto sz = dset.size();
    events_.resize(sz);
    for (size_t i = 0; i < sz; ++i) {
        events_[i].resize(dset[i].size() / sizeof(std::int16_t));
        memcpy(events_[i].data(), dset[i].data(), dset[i].size());
    }
}

void Event::loadTalk(const std::string &name) {
    std::vector<std::string> talks;
    if (!GrpData::loadData(name, talks)) { return; }
    auto sz = talks.size();
    talks_.resize(sz);
    for (size_t i = 0; i < sz; ++i) {
        auto &t = talks[i];
        for (auto &c: t) {
            if (c) { c = ~c; }
        }
        talks_[i] = util::big5Conv.toUnicode(t.c_str());
    }
    if (core::config.simplifiedChinese()) {
        for (auto &t: talks_) {
            t = util::trad2SimpConv.convert(t);
        }
    }
}

const std::vector<std::int16_t> &Event::event(size_t index) const {
    if (index < events_.size()) {
        return events_[index];
    }
    static std::vector<std::int16_t> empty;
    return empty;
}

const std::wstring &Event::talk(size_t index) const {
    if (index < talks_.size()) {
        return talks_[index];
    }
    static std::wstring empty;
    return empty;
}

}
