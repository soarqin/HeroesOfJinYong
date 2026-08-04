#include "item_slots.hh"

#include "data/consts.hh"

namespace hojy::mem {

bool compactCarryItemSlots(CharacterData &character, int slot) {
    if (slot < 0 || slot >= data::CarryItemCount) { return false; }
    for (int i = slot; i + 1 < data::CarryItemCount; ++i) {
        character.item[i] = character.item[i + 1];
        character.itemCount[i] = character.itemCount[i + 1];
    }
    character.item[data::CarryItemCount - 1] = -1;
    character.itemCount[data::CarryItemCount - 1] = 0;
    return true;
}

}
