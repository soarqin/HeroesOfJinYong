/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 */

#include "talkbox.hh"

#include "texture.hh"
#include "window.hh"
#include "window_command.hh"
#include "core/config.hh"
#include "util/math.hh"

#include <algorithm>
#include <map>
#include <set>

namespace hojy::scene {

void TalkBox::popup(const std::wstring &text, std::int16_t headId, std::int16_t position) {
    sourceText_ = text;
    headId_ = headId;
    position_ = position;
    headTex_ = nullptr;
    headScale_ = util::calcSmallestDivision(rootWidth(), 320);
    pageModel_ = {};
    index_ = 0;
    layoutReady_ = false;
    pendingInput_ = KeyNone;
    requestPresentationRefresh();
}

bool TalkBox::prepareTextResources() {
    if (!renderer_ || !renderer_->ttf()) { return false; }
    auto *ttf = renderer_->ttf();
    return ttf->prepareText(sourceText_)
        && ttf->prepareText(L"。，．…* ");
}

void TalkBox::ensureLayout() {
    if (layoutReady_) { return; }
    const auto *candidateHead = (position_ != 2 && position_ != 3 && headId_ >= 0
                                 && headTextureProvider_)
        ? headTextureProvider_(headId_) : nullptr;
    const auto candidateScale = util::calcSmallestDivision(rootWidth(), 320);
    const auto windowBorder = core::config.windowBorder();
    int headWidth = 0;
    if (candidateHead) {
        headWidth = candidateHead->width() * candidateScale.first
            / candidateScale.second + windowBorder * 2;
    }
    const auto maximumLineWidth = width_ - headWidth - windowBorder * 3;
    auto *ttf = renderer_->ttf();
    const auto lineCapacity = (height_ * 2 / 5 - windowBorder * 2
                               + TextLineSpacing)
        / (ttf->fontSize() + TextLineSpacing);
    std::set<wchar_t> characters(sourceText_.begin(), sourceText_.end());
    characters.insert(L'。');
    characters.insert(L'…');
    std::map<wchar_t, int> advances;
    bool metricsReady = true;
    for (const auto ch: characters) {
        if (ch == L'*' || ch < 32 || ch > 0 && ch < 17) { continue; }
        int advance = 0;
        if (!ttf->measureCharAdvance(ch, advance)) {
            metricsReady = false;
            break;
        }
        advances.emplace(ch, advance);
    }
    logic::TalkPageModel candidateModel;
    if (!metricsReady || !logic::buildTalkPageModel(
            sourceText_, maximumLineWidth, lineCapacity, advances, candidateModel)) {
        candidateModel.lines.clear();
        if (!sourceText_.empty()) { candidateModel.lines.push_back(sourceText_); }
        candidateModel.linesPerPage = candidateModel.lines.empty() ? 0 : 1;
    }
    pageModel_ = std::move(candidateModel);
    headTex_ = candidateHead;
    headScale_ = candidateScale;
    layoutReady_ = true;
}

void TalkBox::consumeKeyIntent(Node::Key key) {
    pendingInput_ = key;
}

void TalkBox::applyInputLogic() {
    const auto key = pendingInput_;
    if (key == KeyNone || !layoutReady_) { return; }
    pendingInput_ = KeyNone;
    switch (key) {
    case KeyOK: case KeySpace: case KeyCancel:
        if (index_ + pageModel_.linesPerPage < pageModel_.lines.size()) {
            index_ += pageModel_.linesPerPage;
            while (index_ < pageModel_.lines.size()
                   && pageModel_.lines[index_].empty()) {
                ++index_;
            }
            if (index_ < pageModel_.lines.size()) {
                requestPresentationRefresh();
                break;
            }
        }
        postSceneCommand(this, [](SceneCommandContext &context) { context.endPopup(); });
        break;
    default:
        break;
    }
}

void TalkBox::makeCache() {
    int rowHeight;
    int headX = 0, headY = 0, headW = 0, headH = 0;
    int textX = 0, textY = 0, textW = 0, textH = 0;
    const auto windowBorder = core::config.windowBorder();

    if (headTex_) {
        headW = headTex_->width() * headScale_.first / headScale_.second + windowBorder * 2;
        headH = headTex_->height() * headScale_.first / headScale_.second + windowBorder * 2;
    }
    auto *ttf = renderer_->ttf();
    rowHeight = ttf->fontSize() + TextLineSpacing;
    textW = headTex_ ? width_ - headW - windowBorder : width_;
    const auto size = static_cast<int>(pageModel_.lines.size());
    const auto lines = std::max(
        0, std::min(pageModel_.linesPerPage, size - index_));
    textH = rowHeight * lines + windowBorder * 2 - TextLineSpacing;
    if (position_ % 2) {
        if (headTex_) { headY = height_ - headH; }
        textY = height_ - textH;
    } else {
        textY = 0;
    }
    if (position_ == 1 || position_ == 4) {
        if (headTex_) { headX = width_ - headW; }
        textX = 0;
    } else if (headTex_) {
        headX = 0;
        textX = headW + windowBorder;
    }

    cacheBegin();
    renderer_->clear(0, 0, 0, 0);
    if (headTex_) {
        renderer_->fillRoundedRect(headX, headY, headW, headH, windowBorder, 64, 64, 64, 208);
        renderer_->drawRoundedRect(headX, headY, headW, headH, windowBorder, 224, 224, 224, 255);
        renderer_->renderTexture(headTex_, headX + windowBorder, headY + windowBorder, headScale_, true);
    }
    int x = windowBorder + textX;
    int y = windowBorder + textY;
    renderer_->fillRoundedRect(textX, textY, textW, textH, windowBorder, 64, 64, 64, 208);
    renderer_->drawRoundedRect(textX, textY, textW, textH, windowBorder, 224, 224, 224, 255);
    ttf->setColor(220, 220, 220);
    for (int remaining = lines, index = index_; remaining && index < size;
         --remaining, ++index, y += rowHeight) {
        ttf->renderPrepared(pageModel_.lines[index], x, y, true);
    }
    cacheEnd();
}

}
