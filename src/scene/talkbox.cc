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
    text_.clear();
    size_t idx = 0;
    std::wstring line;
    std::vector<std::wstring> lines;
    size_t tlen = text.length();
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
        /* Break lines, on either real line end or period */
        if (len < 12 || (line.back() == L'．' && *(line.end() - 2) != L'．' && (idx + 2 >= tlen || text[idx + 1] != L'．'))) {
            lines.emplace_back(line);
            line.clear();
        }
        idx = pos + 1;
        while (idx < tlen && text[idx] == 12288/*space*/) {
            ++idx;
        }
    }

    headTex_ = (position != 2 && position != 3 && headId >= 0) ? gWindow->headTexture(headId) : nullptr;
    int headW = 0;
    if (headTex_) {
        headW = headTex_->width() * 2 + SubWindowBorder * 2;
    }

    auto *ttf = renderer_->ttf();
    size_t widthMax = width_ - headW - 10 - SubWindowBorder * 2;
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
                    /* convert to ellipsis */
                    ch = L'…';
                } else {
                    /* this is real period */
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
    index_ = 0;
    dispLines_ = (height_ * 2 / 5 - SubWindowBorder * 2 + TextLineSpacing) / (ttf->fontSize() + TextLineSpacing);
    if (dispLines_ > text_.size()) { dispLines_ = text_.size(); }

    position_ = position;
    update();
}

void TalkBox::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeyOK: case KeySpace: case KeyCancel:
        if (index_ + dispLines_ < text_.size()) {
            index_ += dispLines_;
            /* skip start empty lines */
            while (index_ < text_.size() && text_[index_].empty()) {
                ++index_;
            }
            if (index_ < text_.size()) {
                update();
                break;
            }
        }
        gWindow->endPopup();
        break;
    default:
        break;
    }
}

void TalkBox::makeCache() {
    int rowHeight;
    int headX, headY, headW = 0, headH = 0;
    int textX, textY, textW, textH;

    if (headTex_) {
        headW = headTex_->width() * 2 + SubWindowBorder * 2;
        headH = headTex_->height() * 2 + SubWindowBorder * 2 - TextLineSpacing;
    }
    auto *ttf = renderer_->ttf();
    rowHeight = ttf->fontSize() + TextLineSpacing;
    if (headTex_) {
        textW = width_ - headW - 10;
    } else {
        textW = width_;
    }
    auto sz = int(text_.size());
    auto lines = index_ + dispLines_ > sz ? sz - index_ : dispLines_;
    textH = rowHeight * lines + SubWindowBorder * 2 - TextLineSpacing;
    if (position_ % 2) {
        if (headTex_) {
            headY = height_ - headH;
        }
        textY = height_ - textH;
    } else {
        if (headTex_) {
            headY = 0;
        }
        textY = 0;
    }
    if (position_ == 1 || position_ == 4) {
        if (headTex_) {
            headX = width_ - headW;
        }
        textX = 0;
    } else {
        if (headTex_) {
            headX = headY = 0;
            textX = headW + 10;
        } else {
            textX = 0;
        }
    }

    cacheBegin();
    renderer_->clear(0, 0, 0, 0);
    if (headTex_) {
        renderer_->fillRoundedRect(headX, headY, headW, headH, RoundedRectRad, 64, 64, 64, 208);
        renderer_->drawRoundedRect(headX, headY, headW, headH, RoundedRectRad, 224, 224, 224, 255);
        renderer_->renderTexture(headTex_, float(headX + SubWindowBorder), float(headY + SubWindowBorder), 2., true);
    }

    int x = SubWindowBorder + textX;
    int y = SubWindowBorder + textY;
    renderer_->fillRoundedRect(textX, textY, textW, textH, RoundedRectRad, 64, 64, 64, 208);
    renderer_->drawRoundedRect(textX, textY, textW, textH, RoundedRectRad, 224, 224, 224, 255);
    ttf->setColor(220, 220, 220);
    for (size_t i = dispLines_, idx = index_; i && idx < sz; --i, ++idx, y += rowHeight) {
        ttf->render(text_[idx], x, y, true);
    }
    cacheEnd();
}

}
