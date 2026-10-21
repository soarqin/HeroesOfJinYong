#pragma once

#include "input.hh"

#include <memory>

namespace hojy::scene {

class WarfieldInputContext {
public:
    virtual ~WarfieldInputContext() = default;
    virtual void queueMoveCursor(InputKey key) = 0;
    virtual void queueConfirmMove() = 0;
    virtual void queueConfirmAttack() = 0;
    virtual void queueCancelSelection() = 0;
    virtual void queueCancelAutoControl() = 0;
};

class WarfieldInputExecutionContext {
public:
    virtual ~WarfieldInputExecutionContext() = default;
    virtual void executeMoveCursor(InputKey key) = 0;
    virtual void executeConfirmMove() = 0;
    virtual void executeConfirmAttack() = 0;
    virtual void executeCancelSelection() = 0;
    virtual void executeCancelAutoControl() = 0;
};

class WarfieldInputAction {
public:
    virtual ~WarfieldInputAction() = default;
    virtual void execute(WarfieldInputExecutionContext &context) const = 0;
};

class MoveCursorAction final : public WarfieldInputAction {
public:
    explicit MoveCursorAction(InputKey key): key_(key) {}
    void execute(WarfieldInputExecutionContext &context) const override {
        context.executeMoveCursor(key_);
    }

private:
    InputKey key_ = InputKey::None;
};

class ConfirmMoveAction final : public WarfieldInputAction {
public:
    void execute(WarfieldInputExecutionContext &context) const override {
        context.executeConfirmMove();
    }
};

class ConfirmAttackAction final : public WarfieldInputAction {
public:
    void execute(WarfieldInputExecutionContext &context) const override {
        context.executeConfirmAttack();
    }
};

class CancelSelectionAction final : public WarfieldInputAction {
public:
    void execute(WarfieldInputExecutionContext &context) const override {
        context.executeCancelSelection();
    }
};

class CancelAutoControlAction final : public WarfieldInputAction {
public:
    void execute(WarfieldInputExecutionContext &context) const override {
        context.executeCancelAutoControl();
    }
};

class WarfieldInputMode {
public:
    virtual ~WarfieldInputMode() = default;
    virtual void consume(WarfieldInputContext &context, InputKey key) const = 0;
};

class PassiveWarfieldInputMode final : public WarfieldInputMode {
public:
    void consume(WarfieldInputContext &context, InputKey key) const override;
};

class MoveSelectingInputMode final : public WarfieldInputMode {
public:
    void consume(WarfieldInputContext &context, InputKey key) const override;
};

class AttackSelectingInputMode final : public WarfieldInputMode {
public:
    void consume(WarfieldInputContext &context, InputKey key) const override;
};

}
