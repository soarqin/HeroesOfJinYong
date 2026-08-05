#include "warfield_input_mode.hh"

namespace hojy::scene {

namespace {

bool isDirectional(InputKey key) {
    return key == InputKey::Up || key == InputKey::Down
        || key == InputKey::Left || key == InputKey::Right;
}

void consumeSelection(WarfieldInputContext &context, InputKey key,
                      void (WarfieldInputContext::*confirm)()) {
    if (isDirectional(key)) {
        context.queueMoveCursor(key);
        return;
    }
    if (key == InputKey::Accept || key == InputKey::Space) {
        (context.*confirm)();
        return;
    }
    if (key == InputKey::Cancel) {
        context.queueCancelSelection();
    }
}

}

void PassiveWarfieldInputMode::consume(WarfieldInputContext &context,
                                       InputKey key) const {
    if (key == InputKey::Cancel) {
        context.queueCancelAutoControl();
    }
}

void MoveSelectingInputMode::consume(WarfieldInputContext &context,
                                     InputKey key) const {
    consumeSelection(context, key, &WarfieldInputContext::queueConfirmMove);
}

void AttackSelectingInputMode::consume(WarfieldInputContext &context,
                                       InputKey key) const {
    consumeSelection(context, key, &WarfieldInputContext::queueConfirmAttack);
}

}
