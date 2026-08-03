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
#include <utility>

namespace hojy::data {

Event gEvent;

bool Event::loadEvent(const std::string &name) {
    GrpData::DataSet dset;
    if (!GrpData::loadData(name, dset)) { return false; }
    auto sz = dset.size();
    std::vector<std::vector<std::int16_t>> events(sz);
    for (size_t i = 0; i < sz; ++i) {
        if (dset[i].size() % sizeof(std::int16_t) != 0) { return false; }
        events[i].resize(dset[i].size() / sizeof(std::int16_t));
        if (!dset[i].empty()) {
            memcpy(events[i].data(), dset[i].data(), dset[i].size());
        }
    }
    events_ = std::move(events);
    return true;
}

bool Event::loadTalk(const std::string &name) {
    GrpData::DataSet origTalks;
    if (!GrpData::loadData(name, origTalks)) { return false; }
    auto sz = origTalks.size();
    std::vector<std::wstring> talks(sz);
    for (size_t i = 0; i < sz; ++i) {
        auto &t = origTalks[i];
        for (auto &c: t) {
            if (c) { c = ~c; }
        }
        talks[i] = util::big5Conv.toUnicode(t.c_str());
    }
    if (core::config.simplifiedChinese()) {
        for (auto &t: talks) {
            t = util::trad2SimpConv.convert(t);
        }
    }
    origTalks_ = std::move(origTalks);
    talks_ = std::move(talks);
    return true;
}

const std::vector<std::int16_t> &Event::event(size_t index) const {
    if (index < events_.size()) {
        return events_[index];
    }
    static std::vector<std::int16_t> empty;
    return empty;
}

const std::string &Event::origTalk(size_t index) const {
    if (index < origTalks_.size()) {
        return origTalks_[index];
    }
    static std::string empty;
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
