#pragma once

#include "input.hh"

#include <memory>
#include <string>

namespace hojy::scene {

class TitleInputExecutionContext {
public:
    virtual ~TitleInputExecutionContext() = default;
    virtual void executeMoveSelection(int delta) = 0;
    virtual void executeActivateMainSelection() = 0;
    virtual void executeActivateLoadSelection() = 0;
    virtual void executeReturnToMainMenu() = 0;
    virtual void executeAppendName(std::wstring text) = 0;
    virtual void executeEraseName() = 0;
    virtual void executeSubmitName() = 0;
    virtual void executeSelectConfirmation(int index) = 0;
    virtual void executeActivateConfirmation() = 0;
    virtual void executeRerollCandidate() = 0;
};

class TitleInputAction {
public:
    virtual ~TitleInputAction() = default;
    virtual void execute(TitleInputExecutionContext &context) const = 0;
};

class TitleInputMode {
public:
    virtual ~TitleInputMode() = default;
    [[nodiscard]] virtual std::unique_ptr<TitleInputAction>
    keyAction(InputKey key) const = 0;
    [[nodiscard]] virtual std::unique_ptr<TitleInputAction>
    textAction(std::wstring text) const;
};

class TitleMainMenuInputMode final : public TitleInputMode {
public:
    [[nodiscard]] std::unique_ptr<TitleInputAction>
    keyAction(InputKey key) const override;
};

class TitleLoadInputMode final : public TitleInputMode {
public:
    [[nodiscard]] std::unique_ptr<TitleInputAction>
    keyAction(InputKey key) const override;
};

class TitleNameInputMode final : public TitleInputMode {
public:
    [[nodiscard]] std::unique_ptr<TitleInputAction>
    keyAction(InputKey key) const override;
    [[nodiscard]] std::unique_ptr<TitleInputAction>
    textAction(std::wstring text) const override;
};

class TitleConfirmationInputMode final : public TitleInputMode {
public:
    [[nodiscard]] std::unique_ptr<TitleInputAction>
    keyAction(InputKey key) const override;
};

}
