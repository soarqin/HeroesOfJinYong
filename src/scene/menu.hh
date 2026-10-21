#pragma once

#include "logic/menu.hh"
#include "nodewithcache.hh"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace hojy::scene {

class Menu: public NodeWithCache, protected MenuInputExecutionContext {
public:
    using NodeWithCache::NodeWithCache;

    [[nodiscard]] int currIndex() const { return currIndex_; }
    [[nodiscard]] std::int32_t currEntryId() const;
    inline void setTitle(const std::wstring &title) { title_ = title; }
    void enableCheckBox(bool enabled);
    virtual void enableHorizonal(bool enabled);
    void popup(const std::vector<std::wstring> &items, int defaultIndex = 0);
    void popup(const std::vector<std::wstring> &items,
               const std::vector<std::wstring> &values,
               int defaultIndex = 0);
    void popup(const MenuEntries &entries, int defaultIndex = 0);
    void setEntryIds(const std::vector<std::int32_t> &ids);
    void setEntryEnabledById(std::int32_t entryId, bool enabled);
    void setSelectionSink(std::shared_ptr<MenuSelectionSink> sink);
    void checkItem(std::size_t index, bool check);
    [[nodiscard]] bool itemChecked(std::size_t index) const;

    void applyInputLogic() override;
    void consumeKeyIntent(Key key) override;

protected:
    // MenuInputExecutionContext
    [[nodiscard]] int currentIndex() const noexcept override { return currIndex_; }
    [[nodiscard]] int entryCount() const noexcept override {
        return static_cast<int>(items_.size());
    }
    [[nodiscard]] bool checkboxEnabled() const noexcept override { return checkbox_; }
    void moveSelection(int delta, bool wrap) override;
    void selectIndex(int index) override;
    [[nodiscard]] std::int32_t entryIdAt(int index) const override;
    void submit(MenuGesture gesture) override;

    void setInputMode(std::unique_ptr<MenuInputMode> mode);
    [[nodiscard]] virtual std::unique_ptr<MenuInputMode>
    makeDefaultInputMode() const;
    void markSelectionDirty() { requestPresentationRefresh(); }

private:
    bool prepareTextResources() override;
    void ensureLayout() override;
    void makeCache() override;

protected:
    std::wstring title_;
    std::vector<std::wstring> items_;
    std::vector<std::wstring> values_;
    std::vector<std::int32_t> entryIds_;
    std::vector<bool> entryEnabled_;
    std::vector<bool> selected_;
    int currIndex_ = 0;
    bool checkbox_ = false;
    bool horizonal_ = false;
    std::shared_ptr<MenuSelectionSink> selectionSink_;
    std::unique_ptr<MenuInputMode> inputMode_;
    std::uint64_t nextSelectionToken_ = 1;
    Key pendingInput_ = KeyNone;
};

class MenuTextList: public Menu {
public:
    using Menu::Menu;
};

class MenuYesNo: public Menu {
public:
    using Menu::Menu;

    void enableHorizonal(bool enabled) override;
    void popupWithYesNo();

protected:
    [[nodiscard]] std::unique_ptr<MenuInputMode>
    makeDefaultInputMode() const override;

private:
    bool yesNoInputMode_ = false;
};

class MenuOption: public Menu {
public:
    using Menu::Menu;

    void setValueById(std::int32_t entryId, const std::wstring &value);

protected:
    [[nodiscard]] std::unique_ptr<MenuInputMode>
    makeDefaultInputMode() const override;
};

}
