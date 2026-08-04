#include "world/action.hh"
#include "test_support.hh"

#include <iostream>

namespace {

hojy::world::state::CharacterData carriedItems() {
    hojy::world::state::CharacterData character{};
    for (int i = 0; i < hojy::content::CarryItemCount; ++i) {
        character.item[i] = static_cast<std::int16_t>(10 + i);
        character.itemCount[i] = 1;
    }
    return character;
}

void removesEverySlotSafely() {
    for (int slot = 0; slot < hojy::content::CarryItemCount; ++slot) {
        auto character = carriedItems();
        HOJY_CHECK_EQ(hojy::world::state::consumeNpcItem(&character, character.item[slot]), true);
        for (int i = 0; i < slot; ++i) {
            HOJY_CHECK_EQ(character.item[i], 10 + i);
            HOJY_CHECK_EQ(character.itemCount[i], 1);
        }
        for (int i = slot; i + 1 < hojy::content::CarryItemCount; ++i) {
            HOJY_CHECK_EQ(character.item[i], 11 + i);
            HOJY_CHECK_EQ(character.itemCount[i], 1);
        }
        HOJY_CHECK_EQ(character.item[hojy::content::CarryItemCount - 1], -1);
        HOJY_CHECK_EQ(character.itemCount[hojy::content::CarryItemCount - 1], 0);
    }
}

void decrementsAndRejectsEmptySlots() {
    auto character = carriedItems();
    character.itemCount[1] = 2;
    HOJY_CHECK_EQ(hojy::world::state::consumeNpcItem(&character, character.item[1]), true);
    HOJY_CHECK_EQ(character.item[1], 11);
    HOJY_CHECK_EQ(character.itemCount[1], 1);

    character.itemCount[1] = 0;
    HOJY_CHECK_EQ(hojy::world::state::consumeNpcItem(&character, character.item[1]), false);
    HOJY_CHECK_EQ(hojy::world::state::consumeNpcItem(nullptr, 11), false);
}

void consumesTheSelectedDuplicateSlotInsteadOfTheFirstMatchingId() {
    hojy::world::state::CharacterData character{};
    for (int i = 0; i < hojy::content::CarryItemCount; ++i) {
        character.item[i] = -1;
        character.itemCount[i] = 0;
    }
    character.item[0] = 42;
    character.item[1] = 43;
    character.item[2] = 42;
    character.itemCount[0] = character.itemCount[1] = character.itemCount[2] = 1;

    HOJY_CHECK_EQ(hojy::world::state::consumeNpcItemAt(&character, 2, 42), true);
    HOJY_CHECK_EQ(character.item[0], 42);
    HOJY_CHECK_EQ(character.item[1], 43);
    HOJY_CHECK_EQ(character.item[2], -1);
    HOJY_CHECK_EQ(character.itemCount[2], 0);

    HOJY_CHECK_EQ(hojy::world::state::consumeNpcItemAt(&character, 0, 99), false);
    HOJY_CHECK_EQ(character.item[0], 42);
    HOJY_CHECK_EQ(character.itemCount[0], 1);
}

}

int main() {
    try {
        removesEverySlotSafely();
        decrementsAndRejectsEmptySlots();
        consumesTheSelectedDuplicateSlotInsteadOfTheFirstMatchingId();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
