#pragma once

#include "input.hh"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace hojy::scene {

enum class MenuGesture : std::uint8_t {
    Activate,
    Toggle,
    AdjustPrevious,
    AdjustNext,
    Cancel,
};

struct MenuSelection final {
    std::uint64_t token = 0;
    std::int32_t entryId = -1;
    MenuGesture gesture = MenuGesture::Activate;
};

struct MenuEntry final {
    std::int32_t id = -1;
    std::wstring label;
    std::wstring value;
    bool enabled = true;
};

using MenuEntries = std::vector<MenuEntry>;
constexpr std::int32_t MenuConfirmEntryId = -1;

class MenuSelectionSink {
public:
    virtual ~MenuSelectionSink() = default;
    virtual void submit(MenuSelection selection) = 0;
};

class MenuAction {
public:
    virtual ~MenuAction() = default;
    virtual void execute(MenuSelection selection) = 0;
};

class ActionMenuController final: public MenuSelectionSink {
public:
    void bind(std::int32_t entryId, std::unique_ptr<MenuAction> action);
    void bindDefault(std::unique_ptr<MenuAction> action);
    void bindCancel(std::unique_ptr<MenuAction> action);
    void submit(MenuSelection selection) override;

private:
    std::map<std::int32_t, std::unique_ptr<MenuAction>> actions_;
    std::unique_ptr<MenuAction> defaultAction_;
    std::unique_ptr<MenuAction> cancelAction_;
};

class MenuInputExecutionContext {
public:
    virtual ~MenuInputExecutionContext() = default;
    [[nodiscard]] virtual int currentIndex() const noexcept = 0;
    [[nodiscard]] virtual int entryCount() const noexcept = 0;
    [[nodiscard]] virtual bool checkboxEnabled() const noexcept = 0;
    virtual void moveSelection(int delta, bool wrap) = 0;
    virtual void selectIndex(int index) = 0;
    [[nodiscard]] virtual std::int32_t entryIdAt(int index) const = 0;
    virtual void submit(MenuGesture gesture) = 0;
};

class MenuInputAction {
public:
    virtual ~MenuInputAction() = default;
    virtual void execute(MenuInputExecutionContext &context) const = 0;
};

class MenuInputMode {
public:
    virtual ~MenuInputMode() = default;
    [[nodiscard]] virtual std::unique_ptr<MenuInputAction>
    keyAction(InputKey key) const = 0;
};

class MoveMenuSelectionAction final: public MenuInputAction {
public:
    MoveMenuSelectionAction(int delta, bool wrap): delta_(delta), wrap_(wrap) {}
    void execute(MenuInputExecutionContext &context) const override {
        context.moveSelection(delta_, wrap_);
    }

private:
    int delta_ = 0;
    bool wrap_ = false;
};

class SelectMenuBoundaryAction final: public MenuInputAction {
public:
    explicit SelectMenuBoundaryAction(bool last): last_(last) {}
    void execute(MenuInputExecutionContext &context) const override {
        context.selectIndex(last_ ? context.entryCount() - 1 : 0);
    }

private:
    bool last_ = false;
};

class SubmitMenuGestureAction final: public MenuInputAction {
public:
    explicit SubmitMenuGestureAction(MenuGesture gesture): gesture_(gesture) {}
    void execute(MenuInputExecutionContext &context) const override {
        context.submit(gesture_);
    }

private:
    MenuGesture gesture_ = MenuGesture::Activate;
};

class VerticalMenuInputMode final: public MenuInputMode {
public:
    [[nodiscard]] std::unique_ptr<MenuInputAction>
    keyAction(InputKey key) const override;
};

class HorizontalMenuInputMode final: public MenuInputMode {
public:
    [[nodiscard]] std::unique_ptr<MenuInputAction>
    keyAction(InputKey key) const override;
};

class YesNoMenuInputMode final: public MenuInputMode {
public:
    [[nodiscard]] std::unique_ptr<MenuInputAction>
    keyAction(InputKey key) const override;
};

class VerticalOptionMenuInputMode final: public MenuInputMode {
public:
    [[nodiscard]] std::unique_ptr<MenuInputAction>
    keyAction(InputKey key) const override;
};

class HorizontalOptionMenuInputMode final: public MenuInputMode {
public:
    [[nodiscard]] std::unique_ptr<MenuInputAction>
    keyAction(InputKey key) const override;
};

}
