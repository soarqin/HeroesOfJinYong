#pragma once

#include "character.hh"

namespace hojy::mem {

// Remove one exhausted NPC carry slot and compact the following slots.
bool compactCarryItemSlots(CharacterData &character, int slot);

}
