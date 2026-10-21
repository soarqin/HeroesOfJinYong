#include "menu.hh"

#include "core/config.hh"
#include "world/strings.hh"

#include <algorithm>
#include <limits>
#include <utility>

namespace hojy::scene {

namespace {

std::vector<std::int32_t> sequentialIds(std::size_t count) {
    std::vector<std::int32_t> ids;
    ids.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        if (index > static_cast<std::size_t>(
                std::numeric_limits<std::int32_t>::max())) {
            ids.push_back(-1);
        } else {
            ids.push_back(static_cast<std::int32_t>(index));
        }
    }
    return ids;
}

}

void Menu::setInputMode(std::unique_ptr<MenuInputMode> mode) {
    inputMode_ = std::move(mode);
}

std::unique_ptr<MenuInputMode> Menu::makeDefaultInputMode() const {
    return horizonal_
        ? std::unique_ptr<MenuInputMode>(new HorizontalMenuInputMode())
        : std::unique_ptr<MenuInputMode>(new VerticalMenuInputMode());
}

void Menu::enableCheckBox(bool enabled) {
    if (checkbox_ == enabled) { return; }
    checkbox_ = enabled;
    if (checkbox_) {
        selected_.assign(items_.size(), false);
        if (entryIds_.size() == items_.size()) {
            items_.emplace_back(GETTEXT(80));
            values_.emplace_back(L"");
            entryIds_.push_back(MenuConfirmEntryId);
            entryEnabled_.push_back(true);
        }
    } else {
        if (!entryIds_.empty() && entryIds_.back() == MenuConfirmEntryId) {
            items_.pop_back();
            if (!values_.empty()) { values_.pop_back(); }
            entryIds_.pop_back();
            if (!entryEnabled_.empty()) { entryEnabled_.pop_back(); }
        }
        selected_.clear();
    }
    currIndex_ = std::clamp(currIndex_, 0, std::max(0, entryCount() - 1));
    requestPresentationRefresh();
}

void Menu::enableHorizonal(bool enabled) {
    horizonal_ = enabled;
    setInputMode(makeDefaultInputMode());
}

void Menu::popup(const std::vector<std::wstring> &items, int defaultIndex) {
    popup(items, {}, defaultIndex);
}

void Menu::popup(const std::vector<std::wstring> &items,
                 const std::vector<std::wstring> &values,
                 int defaultIndex) {
    items_ = items;
    values_ = values;
    entryIds_ = sequentialIds(items_.size());
    entryEnabled_.assign(items_.size(), true);
    selected_.assign(checkbox_ ? items_.size() : 0, false);
    if (checkbox_) {
        items_.emplace_back(GETTEXT(80));
        values_.emplace_back(L"");
        entryIds_.push_back(MenuConfirmEntryId);
        entryEnabled_.push_back(true);
    }
    currIndex_ = defaultIndex < 0
        ? -1
        : std::clamp(
              defaultIndex, 0,
              std::max(0, static_cast<int>(items_.size()) - 1));
    requestPresentationRefresh();
}

void Menu::popup(const MenuEntries &entries, int defaultIndex) {
    items_.clear();
    values_.clear();
    entryIds_.clear();
    entryEnabled_.clear();
    selected_.clear();
    items_.reserve(entries.size());
    values_.reserve(entries.size());
    entryIds_.reserve(entries.size());
    entryEnabled_.reserve(entries.size());
    for (const auto &entry: entries) {
        items_.push_back(entry.label);
        values_.push_back(entry.value);
        entryIds_.push_back(entry.id);
        entryEnabled_.push_back(entry.enabled);
    }
    selected_.assign(checkbox_ ? entries.size() : 0, false);
    if (checkbox_) {
        items_.emplace_back(GETTEXT(80));
        values_.emplace_back(L"");
        entryIds_.push_back(MenuConfirmEntryId);
        entryEnabled_.push_back(true);
    }
    currIndex_ = defaultIndex < 0
        ? -1
        : std::clamp(
              defaultIndex, 0,
              std::max(0, static_cast<int>(items_.size()) - 1));
    requestPresentationRefresh();
}

void Menu::setEntryIds(const std::vector<std::int32_t> &ids) {
    const auto count = checkbox_ && !entryIds_.empty()
        && entryIds_.back() == MenuConfirmEntryId
        ? entryIds_.size() - 1 : entryIds_.size();
    if (ids.size() != count) { return; }
    std::copy(ids.begin(), ids.end(), entryIds_.begin());
}

void Menu::setEntryEnabledById(std::int32_t entryId, bool enabled) {
    for (std::size_t index = 0; index < entryIds_.size(); ++index) {
        if (entryIds_[index] == entryId) {
            if (index >= entryEnabled_.size()) {
                entryEnabled_.resize(entryIds_.size(), true);
            }
            entryEnabled_[index] = enabled;
            requestPresentationRefresh();
            return;
        }
    }
}

void Menu::setSelectionSink(std::shared_ptr<MenuSelectionSink> sink) {
    selectionSink_ = std::move(sink);
}

void Menu::checkItem(std::size_t index, bool check) {
    if (!checkbox_ || index >= selected_.size()) { return; }
    selected_[index] = check;
    requestPresentationRefresh();
}

bool Menu::itemChecked(std::size_t index) const {
    return checkbox_ && index < selected_.size() && selected_[index];
}

std::int32_t Menu::currEntryId() const {
    return entryIdAt(currIndex_);
}

std::int32_t Menu::entryIdAt(int index) const {
    if (index < 0 || static_cast<std::size_t>(index) >= entryIds_.size()) {
        return -1;
    }
    return entryIds_[static_cast<std::size_t>(index)];
}

void Menu::moveSelection(int delta, bool wrap) {
    if (items_.empty() || delta == 0) { return; }
    auto next = currIndex_ + delta;
    const auto count = static_cast<int>(items_.size());
    if (wrap) {
        while (next < 0) { next += count; }
        while (next >= count) { next -= count; }
    } else {
        next = std::clamp(next, 0, count - 1);
    }
    if (next != currIndex_) {
        currIndex_ = next;
        requestPresentationRefresh();
    }
}

void Menu::selectIndex(int index) {
    if (items_.empty()) { return; }
    const auto next = std::clamp(index, 0, static_cast<int>(items_.size()) - 1);
    if (next != currIndex_) {
        currIndex_ = next;
        requestPresentationRefresh();
    }
}

void Menu::submit(MenuGesture gesture) {
    if (gesture == MenuGesture::Cancel) {
        if (selectionSink_) {
            selectionSink_->submit({nextSelectionToken_++, currEntryId(), gesture});
        } else {
            requestPresentationCleanup();
        }
        return;
    }
    if (currIndex_ < 0 || static_cast<std::size_t>(currIndex_) >= items_.size()) {
        return;
    }
    const auto index = static_cast<std::size_t>(currIndex_);
    if (index < entryEnabled_.size() && !entryEnabled_[index]) { return; }
    if (checkbox_ && index < selected_.size()) {
        if (gesture == MenuGesture::Activate) {
            selected_[index] = !selected_[index];
            requestPresentationRefresh();
            if (selectionSink_) {
                selectionSink_->submit({
                    nextSelectionToken_++, currEntryId(), MenuGesture::Toggle});
            }
            return;
        }
    }
    if (selectionSink_) {
        selectionSink_->submit({nextSelectionToken_++, currEntryId(), gesture});
    }
}

void Menu::consumeKeyIntent(Node::Key key) {
    pendingInput_ = key;
}

void Menu::applyInputLogic() {
    const auto key = pendingInput_;
    pendingInput_ = KeyNone;
    if (!inputMode_) {
        setInputMode(makeDefaultInputMode());
    }
    auto action = inputMode_->keyAction(key);
    if (action) { action->execute(*this); }
}

void Menu::ensureLayout() {
    const auto windowBorder = core::config.windowBorder();
    int h = 0, w = 0, w2 = 0;
    const auto lines = static_cast<int>(items_.size());
    const auto fontSize = renderer_->fontSize();
    const auto rowHeight = fontSize + TextLineSpacing;
    const auto hasValueColumn = std::any_of(
        values_.begin(), values_.end(),
        [](const auto &value) { return !value.empty(); });
    if (horizonal_) {
        for (const auto &item: items_) {
            w += renderer_->ttf()->preparedStringWidth(item) + windowBorder;
        }
        w += windowBorder;
        h = rowHeight * (title_.empty() ? 1 : 2)
            + windowBorder * 2 - TextLineSpacing;
    } else {
        for (const auto &item: items_) {
            w = std::max(w, renderer_->ttf()->preparedStringWidth(item));
        }
        if (hasValueColumn) {
            for (const auto &value: values_) {
                w2 = std::max(w2, renderer_->ttf()->preparedStringWidth(value));
            }
            w += w2 + windowBorder;
        }
        auto totalLines = lines;
        if (!title_.empty()) {
            ++totalLines;
            w = std::max(w, renderer_->ttf()->preparedStringWidth(title_));
        }
        if (checkbox_) { w += renderer_->ttf()->preparedStringWidth(L"*"); }
        w += windowBorder * 2;
        h = rowHeight * totalLines + windowBorder * 2 - TextLineSpacing;
    }
    width_ = w;
    height_ = h;
}

bool Menu::prepareTextResources() {
    auto *ttf = renderer_->ttf();
    bool ready = ttf->prepareText(title_);
    for (const auto &item: items_) { ready = ttf->prepareText(item) && ready; }
    for (const auto &value: values_) { ready = ttf->prepareText(value) && ready; }
    return ttf->prepareText(L"*") && ready;
}

void Menu::makeCache() {
    auto *ttf = renderer_->ttf();
    const auto windowBorder = core::config.windowBorder();
    int x = windowBorder, y = windowBorder, h, w = 0, wfill = 0, x2 = 0, w2 = 0;
    const auto lines = static_cast<int>(items_.size());
    const auto fontSize = ttf->fontSize();
    const auto rowHeight = fontSize + TextLineSpacing;
    const auto hasValueColumn = std::any_of(
        values_.begin(), values_.end(),
        [](const auto &value) { return !value.empty(); });
    std::vector<std::pair<int, int>> itemsOff;
    bool drawValue = false;
    if (horizonal_) {
        for (const auto &item: items_) {
            const auto sw = ttf->preparedStringWidth(item);
            itemsOff.emplace_back(std::make_pair(w, sw));
            w += sw + windowBorder;
        }
        w += windowBorder;
        h = rowHeight * (title_.empty() ? 1 : 2)
            + windowBorder * 2 - TextLineSpacing;
    } else {
        for (const auto &item: items_) {
            w = std::max(w, ttf->preparedStringWidth(item));
        }
        if (hasValueColumn) {
            drawValue = true;
            x2 = x + w + windowBorder;
            for (const auto &value: values_) {
                w2 = std::max(w2, ttf->preparedStringWidth(value));
            }
            w += w2 + windowBorder;
        }
        auto totalLines = lines;
        if (!title_.empty()) {
            ++totalLines;
            w = std::max(w, ttf->preparedStringWidth(title_));
        }
        wfill = w;
        if (checkbox_) {
            const auto checkBoxW = ttf->preparedStringWidth(L"*");
            x += checkBoxW;
            x2 += checkBoxW;
            w += checkBoxW;
        }
        w += windowBorder * 2;
        h = rowHeight * totalLines + windowBorder * 2 - TextLineSpacing;
    }
    width_ = w;
    height_ = h;

    cacheBegin();
    renderer_->clear(0, 0, 0, 0);
    renderer_->fillRoundedRect(0, 0, w, h, windowBorder, 64, 64, 64, 208);
    renderer_->drawRoundedRect(0, 0, w, h, windowBorder, 224, 224, 224, 255);
    if (!title_.empty()) {
        ttf->setColor(236, 200, 40);
        ttf->renderPrepared(title_, x, y, true);
        y += rowHeight;
    }
    if (horizonal_) {
        for (int i = 0; i < lines; ++i) {
            if (i == currIndex_) {
                ttf->setColor(236, 236, 236);
                renderer_->fillRoundedRect(
                    x + itemsOff[i].first - 2, y - 2, itemsOff[i].second + 4,
                    fontSize + 4, 2, 96, 96, 96, 192);
            } else {
                ttf->setColor(252, 148, 16);
            }
            ttf->renderPrepared(items_[i], x + itemsOff[i].first, y, true);
        }
    } else {
        for (int i = 0; i < lines; ++i, y += rowHeight) {
            if (i == currIndex_) {
                ttf->setColor(236, 236, 236);
                renderer_->fillRoundedRect(
                    x - 2, y - 2, wfill + 4, fontSize + 4, 2,
                    96, 96, 96, 192);
            } else if (i < static_cast<int>(entryEnabled_.size())
                       && !entryEnabled_[i]) {
                ttf->setColor(128, 128, 128);
            } else {
                ttf->setColor(252, 148, 16);
            }
            if (checkbox_ && i < static_cast<int>(selected_.size())
                && selected_[i]) {
                ttf->renderPrepared(L"*", windowBorder, y, true);
            }
            ttf->renderPrepared(items_[i], x, y, true);
            if (drawValue && i < static_cast<int>(values_.size())) {
                ttf->renderPrepared(values_[i], x2, y, true);
            }
        }
    }
    cacheEnd();
}

void MenuYesNo::enableHorizonal(bool enabled) {
    horizonal_ = enabled;
    setInputMode(makeDefaultInputMode());
}

std::unique_ptr<MenuInputMode> MenuYesNo::makeDefaultInputMode() const {
    if (yesNoInputMode_) {
        return std::make_unique<YesNoMenuInputMode>();
    }
    return Menu::makeDefaultInputMode();
}

void MenuYesNo::popupWithYesNo() {
    yesNoInputMode_ = true;
    setInputMode(std::make_unique<YesNoMenuInputMode>());
    popup({GETTEXT(78), GETTEXT(79)}, -1);
}

std::unique_ptr<MenuInputMode> MenuOption::makeDefaultInputMode() const {
    return horizonal_
        ? std::unique_ptr<MenuInputMode>(new HorizontalOptionMenuInputMode())
        : std::unique_ptr<MenuInputMode>(new VerticalOptionMenuInputMode());
}

void MenuOption::setValueById(
        std::int32_t entryId, const std::wstring &value) {
    for (std::size_t index = 0; index < entryIds_.size(); ++index) {
        if (entryIds_[index] == entryId) {
            if (index >= values_.size()) { values_.resize(entryIds_.size()); }
            values_[index] = value;
            requestPresentationRefresh();
            return;
        }
    }
}

}
