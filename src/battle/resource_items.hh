#pragma once

#include <optional>
#include <vector>

namespace hojy::battle {

enum class ResourceItemKind {
    Hp,
    Mp,
    Stamina,
    Poisoned,
};

struct ResourceItemOption {
    int itemId = -1;
    int addHp = 0;
    int addMp = 0;
    int addStamina = 0;
    int addPoisoned = 0;
};

// The DOS routines scan their source array and return the first item with the
// required sign.  The caller controls the source order (NPC carry slots or
// bag save slots).
std::optional<int> chooseFirstResourceItem(
    const std::vector<ResourceItemOption> &items, ResourceItemKind kind);

}
