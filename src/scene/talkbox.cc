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
#include "window.hh"

namespace hojy::scene {

void TalkBox::popup(const std::wstring &text, std::int16_t headId, std::int16_t position) {
    size_t idx = 0;
    std::wstring line;
    std::vector<std::wstring> lines;
    size_t tlen = text.length();;
    while (idx < tlen) {
        auto pos = text.find(L'*', idx);
        if (pos == std::wstring::npos) {
            line.append(text.substr(idx));
            while (!line.empty() && line.back() == 0) {
                line.erase(line.end() - 1);
            }
            lines.emplace_back(line);
            break;
        }
        auto len = pos - idx;
        auto seg = text.substr(idx, len);
        while (!seg.empty() && seg.back() == 12288/*space*/) {
            seg.erase(seg.end() - 1);
        }
        while (!seg.empty() && seg.front() == 12288/*space*/) {
            seg.erase(seg.begin());
        }
        line.append(seg);
        if (len < 12 || (line.back() == L'．' && *(line.end() - 2) != L'．' && (idx + 2 >= tlen || text[idx + 1] != L'．'))) {
            lines.emplace_back(line);
            line.clear();
        }
        idx = pos + 1;
        while (idx < tlen && text[idx] == 12288/*space*/) {
            ++idx;
        }
    }
    auto *ttf = renderer_->ttf();
    size_t widthMax = width_ - 100;
    for (auto &l: lines) {
        size_t len = l.length();
        if (!len) {
            text_.emplace_back(L"");
            continue;
        }
        idx = 0;
        size_t w = 0;
        for (size_t i = 0; i < len; ++i) {
            auto &ch = l[i];
            if (ch == L'．') {
                if ((i > 0 && l[i - 1] == L'…') || (i + 1 < len && l[i + 1] == L'．')) {
                    ch = L'…';
                } else {
                    ch = L'。';
                }
            }
            std::uint8_t width;
            std::int8_t y0, y1;
            ttf->charDimension(ch, width, y0, y1);
            w += width;
            if (w > widthMax) {
                text_.emplace_back(l.substr(idx, i - idx));
                idx = i;
                w = width;
            }
        }
        if (idx < len) {
            text_.emplace_back(l.substr(idx));
        }
    }
    headTex_ = headId >= 0 ? gHeadTextureMgr[headId] : nullptr;
    position_ = position;

    index_ = 0;
    dispLines_ = (height_ - 10 + 3) / (ttf->fontSize() + 3);
}

void TalkBox::render() {
    if (headTex_) {
        renderer_->fillRect(x_, y_, headTex_->width() * 2 + 10, headTex_->height() * 2 + 10, 192, 192, 192, 96);
        renderer_->renderTexture(headTex_, float(x_ + 5), float(y_ + 5), 2., true);
    }

    auto *ttf = renderer_->ttf();
    size_t sz = text_.size();
    int y = 5 + y_;
    int rowsize = ttf->fontSize() + 3;
    int x = 100 + x_;
    renderer_->fillRect(95 + x_, y_, width_ - 95, rowsize * dispLines_ + 10 + 3, 192, 192, 192, 96);
    for (size_t i = dispLines_, idx = index_; i && idx < sz; --i, ++idx, y += rowsize) {
        ttf->render(text_[idx], x, y, 0);
    }
}

void TalkBox::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeyOK: case KeyCancel:
        if (index_ + dispLines_ < text_.size()) {
            index_ += dispLines_;
        } else {
            gWindow->closePopup();
        }
        break;
    default:
        break;
    }
}

}
