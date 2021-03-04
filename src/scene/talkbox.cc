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

#include "talkbox.hh"

#include "texture.hh"

#include "util/conv.hh"

namespace hojy::scene {

void TalkBox::popup(const std::string &text, std::int16_t headId, std::int16_t position) {
    popup(util::big5Conv.toUnicode(text), headId, position);
}

void TalkBox::popup(const std::wstring &text, std::int16_t headId, std::int16_t position) {
    auto idx = 0;
    std::wstring line;
    while (true) {
        auto pos = text.find(L'*', idx);
        if (pos == std::wstring::npos) {
            line.append(text.substr(idx));
            text_.emplace_back(line);
            break;
        }
        auto len = pos - idx;
        line.append(text.substr(idx, len));
        if (len < 12 || line.back() == L'．') {
            text_.emplace_back(line);
            line.clear();
        }
        idx = pos + 1;
    }
    headTex_ = headId >= 0 ? gHeadTextureMgr[headId] : nullptr;
    position_ = position;
}

void TalkBox::render() {
    if (headTex_) {
        renderer_->renderTexture(headTex_, float(x_ + 5), float(y_ + 100), 2.);
    }
}

}
