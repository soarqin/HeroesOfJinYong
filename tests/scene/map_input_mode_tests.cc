#include "scene/logic/map_input.hh"
#include "test_support.hh"

#include <iostream>

namespace {

class RecordingContext final : public hojy::scene::MapInputContext {
public:
    void requestMove(hojy::scene::InputKey key) override { move = key; }
    void requestInteract() override { interacted = true; }
    void requestOpenMenu() override { opened = true; }

    hojy::scene::InputKey move = hojy::scene::InputKey::None;
    bool interacted = false;
    bool opened = false;
};

void testDefaultMapModeCreatesTypedActions() {
    hojy::scene::DefaultMapInputMode mode;
    RecordingContext context;

    auto move = mode.translate(hojy::scene::InputKey::Left);
    auto interact = mode.translate(hojy::scene::InputKey::Accept);
    auto menu = mode.translate(hojy::scene::InputKey::Cancel);
    HOJY_CHECK_EQ(static_cast<bool>(move), true);
    HOJY_CHECK_EQ(static_cast<bool>(interact), true);
    HOJY_CHECK_EQ(static_cast<bool>(menu), true);

    move->execute(context);
    HOJY_CHECK_EQ(context.move, hojy::scene::InputKey::Left);
    interact->execute(context);
    HOJY_CHECK_EQ(context.interacted, true);
    menu->execute(context);
    HOJY_CHECK_EQ(context.opened, true);
}

}

int main() {
    try {
        testDefaultMapModeCreatesTypedActions();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
