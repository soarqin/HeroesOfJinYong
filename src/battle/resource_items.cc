#include "resource_items.hh"

namespace hojy::battle {

std::optional<int> chooseFirstResourceItem(
    const std::vector<ResourceItemOption> &items, ResourceItemKind kind) {
    for (const auto &item: items) {
        bool qualifies = false;
        switch (kind) {
        case ResourceItemKind::Hp:
            qualifies = item.addHp > 0;
            break;
        case ResourceItemKind::Mp:
            qualifies = item.addMp > 0;
            break;
        case ResourceItemKind::Stamina:
            qualifies = item.addStamina > 0;
            break;
        case ResourceItemKind::Poisoned:
            qualifies = item.addPoisoned < 0;
            break;
        }
        if (qualifies) { return item.itemId; }
    }
    return std::nullopt;
}

}
