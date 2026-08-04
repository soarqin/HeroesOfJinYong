#include "item_slots.hh"

#include "content/constants.hh"

namespace hojy::world::state {

bool compactCarryItemSlots(CharacterData &character, int slot) {
    if (slot < 0 || slot >= ::hojy::content::CarryItemCount) { return false; }
    for (int i = slot; i + 1 < ::hojy::content::CarryItemCount; ++i) {
        character.item[i] = character.item[i + 1];
        character.itemCount[i] = character.itemCount[i + 1];
    }
    character.item[::hojy::content::CarryItemCount - 1] = -1;
    character.itemCount[::hojy::content::CarryItemCount - 1] = 0;
    return true;
}

}
