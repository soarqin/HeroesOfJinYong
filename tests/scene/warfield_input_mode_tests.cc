#include "scene/logic/warfield_input_mode.hh"

#include "test_support.hh"

#include <iostream>

namespace {

class RecordingContext final : public hojy::scene::WarfieldInputContext {
public:
    void queueMoveCursor(hojy::scene::InputKey key) override {
        lastMove = key;
        ++moveCount;
    }

    void queueConfirmMove() override { ++confirmMoveCount; }
    void queueConfirmAttack() override { ++confirmAttackCount; }
    void queueCancelSelection() override { ++cancelSelectionCount; }
    void queueCancelAutoControl() override { ++cancelAutoCount; }

    hojy::scene::InputKey lastMove = hojy::scene::InputKey::None;
    int moveCount = 0;
    int confirmMoveCount = 0;
    int confirmAttackCount = 0;
    int cancelSelectionCount = 0;
    int cancelAutoCount = 0;
};

void testPassiveModeOnlyCancelsAutoControl() {
    RecordingContext context;
    hojy::scene::PassiveWarfieldInputMode mode;
    mode.consume(context, hojy::scene::InputKey::Left);
    mode.consume(context, hojy::scene::InputKey::Cancel);
    HOJY_CHECK_EQ(context.moveCount, 0);
    HOJY_CHECK_EQ(context.cancelAutoCount, 1);
}

void testSelectionModesDelegateWithoutStageBranches() {
    RecordingContext context;
    hojy::scene::MoveSelectingInputMode move;
    move.consume(context, hojy::scene::InputKey::Right);
    move.consume(context, hojy::scene::InputKey::Accept);
    move.consume(context, hojy::scene::InputKey::Cancel);
    HOJY_CHECK_EQ(context.lastMove, hojy::scene::InputKey::Right);
    HOJY_CHECK_EQ(context.confirmMoveCount, 1);
    HOJY_CHECK_EQ(context.cancelSelectionCount, 1);

    hojy::scene::AttackSelectingInputMode attack;
    attack.consume(context, hojy::scene::InputKey::Space);
    HOJY_CHECK_EQ(context.confirmAttackCount, 1);
}

}

int main() {
    try {
        testPassiveModeOnlyCancelsAutoControl();
        testSelectionModesDelegateWithoutStageBranches();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
