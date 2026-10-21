#include "title_input_mode.hh"

#include <utility>

namespace hojy::scene {
namespace {

class MoveSelectionAction final : public TitleInputAction {
public:
    explicit MoveSelectionAction(int delta): delta_(delta) {}

    void execute(TitleInputExecutionContext &context) const override {
        context.executeMoveSelection(delta_);
    }

private:
    int delta_ = 0;
};

class ActivateMainSelectionAction final : public TitleInputAction {
public:
    void execute(TitleInputExecutionContext &context) const override {
        context.executeActivateMainSelection();
    }
};

class ActivateLoadSelectionAction final : public TitleInputAction {
public:
    void execute(TitleInputExecutionContext &context) const override {
        context.executeActivateLoadSelection();
    }
};

class ReturnToMainMenuAction final : public TitleInputAction {
public:
    void execute(TitleInputExecutionContext &context) const override {
        context.executeReturnToMainMenu();
    }
};

class AppendNameAction final : public TitleInputAction {
public:
    explicit AppendNameAction(std::wstring text): text_(std::move(text)) {}

    void execute(TitleInputExecutionContext &context) const override {
        context.executeAppendName(text_);
    }

private:
    std::wstring text_;
};

class EraseNameAction final : public TitleInputAction {
public:
    void execute(TitleInputExecutionContext &context) const override {
        context.executeEraseName();
    }
};

class SubmitNameAction final : public TitleInputAction {
public:
    void execute(TitleInputExecutionContext &context) const override {
        context.executeSubmitName();
    }
};

class SelectConfirmationAction final : public TitleInputAction {
public:
    explicit SelectConfirmationAction(int index): index_(index) {}

    void execute(TitleInputExecutionContext &context) const override {
        context.executeSelectConfirmation(index_);
    }

private:
    int index_ = -1;
};

class ActivateConfirmationAction final : public TitleInputAction {
public:
    void execute(TitleInputExecutionContext &context) const override {
        context.executeActivateConfirmation();
    }
};

class RerollCandidateAction final : public TitleInputAction {
public:
    void execute(TitleInputExecutionContext &context) const override {
        context.executeRerollCandidate();
    }
};

std::unique_ptr<TitleInputAction> movementAction(InputKey key) {
    if (key == InputKey::Up) {
        return std::make_unique<MoveSelectionAction>(-1);
    }
    if (key == InputKey::Down) {
        return std::make_unique<MoveSelectionAction>(1);
    }
    return {};
}

}

std::unique_ptr<TitleInputAction>
TitleInputMode::textAction(std::wstring) const {
    return {};
}

std::unique_ptr<TitleInputAction>
TitleMainMenuInputMode::keyAction(InputKey key) const {
    if (auto action = movementAction(key)) { return action; }
    if (key == InputKey::Accept || key == InputKey::Space) {
        return std::make_unique<ActivateMainSelectionAction>();
    }
    return {};
}

std::unique_ptr<TitleInputAction>
TitleLoadInputMode::keyAction(InputKey key) const {
    if (auto action = movementAction(key)) { return action; }
    if (key == InputKey::Accept || key == InputKey::Space) {
        return std::make_unique<ActivateLoadSelectionAction>();
    }
    if (key == InputKey::Cancel) {
        return std::make_unique<ReturnToMainMenuAction>();
    }
    return {};
}

std::unique_ptr<TitleInputAction>
TitleNameInputMode::keyAction(InputKey key) const {
    if (key == InputKey::Backspace) {
        return std::make_unique<EraseNameAction>();
    }
    if (key == InputKey::Accept) {
        return std::make_unique<SubmitNameAction>();
    }
    if (key == InputKey::Cancel) {
        return std::make_unique<ReturnToMainMenuAction>();
    }
    return {};
}

std::unique_ptr<TitleInputAction>
TitleNameInputMode::textAction(std::wstring text) const {
    if (text.empty()) { return {}; }
    return std::make_unique<AppendNameAction>(std::move(text));
}

std::unique_ptr<TitleInputAction>
TitleConfirmationInputMode::keyAction(InputKey key) const {
    if (key == InputKey::Up || key == InputKey::Left) {
        return std::make_unique<SelectConfirmationAction>(0);
    }
    if (key == InputKey::Down || key == InputKey::Right) {
        return std::make_unique<SelectConfirmationAction>(1);
    }
    if (key == InputKey::Accept || key == InputKey::Space) {
        return std::make_unique<ActivateConfirmationAction>();
    }
    if (key == InputKey::Cancel) {
        return std::make_unique<RerollCandidateAction>();
    }
    return {};
}

}
