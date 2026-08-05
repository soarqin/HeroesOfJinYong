#include "world/item_transaction.hh"
#include "test_support.hh"

#include <cstring>
#include <iostream>
#include <string>

namespace {

template<typename T, typename Container>
void loadRecords(Container &container, const T *records, std::size_t count) {
    const auto *bytes = reinterpret_cast<const char *>(records);
    HOJY_CHECK_EQ(container.deserialize(
        std::string(bytes, bytes + sizeof(T) * count)), true);
}

void resetWorld() {
    hojy::world::state::gSaveData = hojy::world::state::SaveData{};
    hojy::world::state::gBag = hojy::world::state::Bag{};
}

void prepareWorld() {
    hojy::world::state::CharacterData characters[2]{};
    for (auto &character: characters) {
        character.equip[0] = character.equip[1] = -1;
        character.learningItem = -1;
        character.hp = 50;
        character.maxHp = 100;
        character.mp = character.attack = character.speed = 100;
        character.defence = character.potential = 100;
    }
    characters[0].id = 0;
    characters[1].id = 1;
    loadRecords(hojy::world::state::gSaveData.charInfo, characters, 2);

    hojy::world::state::ItemData items[2]{};
    items[0].id = 0;
    items[0].itemType = 1;
    items[0].equipType = 0;
    items[0].user = -1;
    items[1].id = 1;
    items[1].itemType = 3;
    items[1].addHp = 20;
    loadRecords(hojy::world::state::gSaveData.itemInfo, items, 2);

    hojy::world::state::gBag.add(1, 1);
}

void candidateCommitIsAtomic() {
    using namespace hojy::world::state;
    const auto beforeEquip = gSaveData.charInfo[0]->equip[0];
    auto candidate = prepareEquipItem(0, 0);
    HOJY_CHECK_EQ(candidate.has_value(), true);
    HOJY_CHECK_EQ(gSaveData.charInfo[0]->equip[0], beforeEquip);
    HOJY_CHECK_EQ(candidate->bagItems().size(), gBag.items().size());
    HOJY_CHECK_EQ(commitItemAction(std::move(*candidate)), true);
    HOJY_CHECK_EQ(gSaveData.charInfo[0]->equip[0], 0);
    HOJY_CHECK_EQ(gSaveData.itemInfo[0]->user, 0);
}

void staleCandidateCannotOverwriteNewerState() {
    using namespace hojy::world::state;
    auto stale = prepareConsumeItem(1, 0);
    HOJY_CHECK_EQ(stale.has_value(), true);
    auto newer = prepareConsumeItem(1, 0);
    HOJY_CHECK_EQ(newer.has_value(), true);
    HOJY_CHECK_EQ(commitItemAction(std::move(*newer)), true);
    HOJY_CHECK_EQ(commitItemAction(std::move(*stale)), false);
}

}

int main() {
    try {
        resetWorld();
        prepareWorld();
        candidateCommitIsAtomic();
        resetWorld();
        prepareWorld();
        staleCandidateCannotOverwriteNewerState();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
