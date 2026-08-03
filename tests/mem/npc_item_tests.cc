/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>
 */

#include "mem/action.hh"
#include "test_support.hh"

#include <iostream>

namespace {

hojy::mem::CharacterData carriedItems() {
    hojy::mem::CharacterData character{};
    for (int i = 0; i < hojy::data::CarryItemCount; ++i) {
        character.item[i] = static_cast<std::int16_t>(10 + i);
        character.itemCount[i] = 1;
    }
    return character;
}

void removesEverySlotSafely() {
    for (int slot = 0; slot < hojy::data::CarryItemCount; ++slot) {
        auto character = carriedItems();
        HOJY_CHECK_EQ(hojy::mem::consumeNpcItem(&character, character.item[slot]), true);
        for (int i = 0; i < slot; ++i) {
            HOJY_CHECK_EQ(character.item[i], 10 + i);
            HOJY_CHECK_EQ(character.itemCount[i], 1);
        }
        for (int i = slot; i + 1 < hojy::data::CarryItemCount; ++i) {
            HOJY_CHECK_EQ(character.item[i], 11 + i);
            HOJY_CHECK_EQ(character.itemCount[i], 1);
        }
        HOJY_CHECK_EQ(character.item[hojy::data::CarryItemCount - 1], -1);
        HOJY_CHECK_EQ(character.itemCount[hojy::data::CarryItemCount - 1], 0);
    }
}

void decrementsAndRejectsEmptySlots() {
    auto character = carriedItems();
    character.itemCount[1] = 2;
    HOJY_CHECK_EQ(hojy::mem::consumeNpcItem(&character, character.item[1]), true);
    HOJY_CHECK_EQ(character.item[1], 11);
    HOJY_CHECK_EQ(character.itemCount[1], 1);

    character.itemCount[1] = 0;
    HOJY_CHECK_EQ(hojy::mem::consumeNpcItem(&character, character.item[1]), false);
    HOJY_CHECK_EQ(hojy::mem::consumeNpcItem(nullptr, 11), false);
}

}

int main() {
    try {
        removesEverySlotSafely();
        decrementsAndRejectsEmptySlots();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
