#include "scene/title.hh"

#include "scene/logic/title_snapshot.hh"
#include "test_support.hh"

#include <iostream>
#include <variant>

namespace {

void testTitleUsesPolymorphicInputModeAndPublishesSnapshot() {
    hojy::scene::Title title(
        static_cast<hojy::scene::Node *>(nullptr), 0, 0, 320, 200);
    HOJY_CHECK_EQ(
        std::holds_alternative<hojy::scene::TitleMainMenuSnapshot>(
            title.screenSnapshot()), true);

    title.consume(hojy::scene::KeyIntent(hojy::scene::InputKey::Down));
    title.dispatchInputLogic();
    const auto &main = std::get<hojy::scene::TitleMainMenuSnapshot>(
        title.screenSnapshot());
    HOJY_CHECK_EQ(main.selectedIndex, 1);

    title.consume(hojy::scene::KeyIntent(hojy::scene::InputKey::Accept));
    title.dispatchInputLogic();
    HOJY_CHECK_EQ(
        std::holds_alternative<hojy::scene::TitleLoadMenuSnapshot>(
            title.screenSnapshot()), true);

    title.consume(hojy::scene::KeyIntent(hojy::scene::InputKey::Cancel));
    title.dispatchInputLogic();
    HOJY_CHECK_EQ(
        std::holds_alternative<hojy::scene::TitleMainMenuSnapshot>(
            title.screenSnapshot()), true);
}

}

int main() {
    try {
        testTitleUsesPolymorphicInputModeAndPublishesSnapshot();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
