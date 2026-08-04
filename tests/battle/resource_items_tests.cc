#include "battle/resource_items.hh"
#include "data/consts.hh"
#include "mem/bag.hh"
#include "mem/item_slots.hh"
#include "mem/savedata.hh"
#include "test_support.hh"

#include <iostream>

namespace hojy::mem {
SaveData gSaveData;
}

namespace {

using hojy::battle::ResourceItemKind;
using hojy::battle::ResourceItemOption;

void testSelectionKeepsInputOrderForEachResourceKind() {
    const std::vector<ResourceItemOption> options{
        {11, 5, 1, 0, 0},
        {22, 20, 10, 8, -5},
        {33, 30, 30, 30, -10},
    };

    HOJY_CHECK_EQ(
        hojy::battle::chooseFirstResourceItem(options, ResourceItemKind::Hp), 11);
    HOJY_CHECK_EQ(
        hojy::battle::chooseFirstResourceItem(options, ResourceItemKind::Mp), 11);
    HOJY_CHECK_EQ(
        hojy::battle::chooseFirstResourceItem(options, ResourceItemKind::Stamina), 22);
    HOJY_CHECK_EQ(
        hojy::battle::chooseFirstResourceItem(options, ResourceItemKind::Poisoned), 22);
}

void testSelectionReturnsEmptyWhenNoQualifyingItemExists() {
    const std::vector<ResourceItemOption> options{{11, 0, 0, 0, 0}};
    HOJY_CHECK_EQ(
        hojy::battle::chooseFirstResourceItem(options, ResourceItemKind::Hp)
            .has_value(), false);
}

void testBagKeepsSaveSlotOrderForBattleScans() {
    for (int i = 0; i < hojy::data::BagItemCount; ++i) {
        hojy::mem::gSaveData.baseInfo->items[i] = {-1, 0};
    }
    hojy::mem::gSaveData.baseInfo->items[0] = {80, 1};
    hojy::mem::gSaveData.baseInfo->items[1] = {10, 2};
    hojy::mem::gBag.syncFromSave();

    const auto &ordered = hojy::mem::gBag.orderedItems();
    HOJY_CHECK_EQ(ordered.size(), 2U);
    HOJY_CHECK_EQ(ordered[0].first, 80);
    HOJY_CHECK_EQ(ordered[1].first, 10);

    hojy::mem::gBag.add(30, 1);
    HOJY_CHECK_EQ(hojy::mem::gBag.orderedItems().back().first, 30);
    hojy::mem::gBag.remove(80, 1);
    HOJY_CHECK_EQ(hojy::mem::gBag.orderedItems().front().first, 10);

    hojy::mem::gBag.syncToSave();
    HOJY_CHECK_EQ(hojy::mem::gSaveData.baseInfo->items[0].id, 10);
    HOJY_CHECK_EQ(hojy::mem::gSaveData.baseInfo->items[1].id, 30);
    HOJY_CHECK_EQ(hojy::mem::gSaveData.baseInfo->items[2].id, -1);
    HOJY_CHECK_EQ(hojy::mem::gSaveData.baseInfo->items[2].count, 0);
}

void testCarrySlotCompactionCopiesWholeSlots() {
    hojy::mem::CharacterData character{};
    for (int i = 0; i < hojy::data::CarryItemCount; ++i) {
        character.item[i] = static_cast<std::int16_t>(100 + i);
        character.itemCount[i] = static_cast<std::int16_t>(i + 1);
    }

    HOJY_CHECK_EQ(hojy::mem::compactCarryItemSlots(character, 0), true);
    HOJY_CHECK_EQ(character.item[0], 101);
    HOJY_CHECK_EQ(character.itemCount[0], 2);
    HOJY_CHECK_EQ(character.item[2], 103);
    HOJY_CHECK_EQ(character.itemCount[2], 4);
    HOJY_CHECK_EQ(character.item[3], -1);
    HOJY_CHECK_EQ(character.itemCount[3], 0);

    character.item[0] = 301;
    character.item[1] = 302;
    character.item[2] = 303;
    character.item[3] = 304;
    character.itemCount[0] = 1;
    character.itemCount[1] = 2;
    character.itemCount[2] = 3;
    character.itemCount[3] = 4;
    HOJY_CHECK_EQ(hojy::mem::compactCarryItemSlots(character, 1), true);
    HOJY_CHECK_EQ(character.item[0], 301);
    HOJY_CHECK_EQ(character.item[1], 303);
    HOJY_CHECK_EQ(character.itemCount[1], 3);
    HOJY_CHECK_EQ(character.item[2], 304);
    HOJY_CHECK_EQ(character.item[3], -1);

    character.item[0] = 201;
    character.item[1] = 202;
    character.item[2] = 203;
    character.item[3] = 204;
    character.itemCount[0] = 1;
    character.itemCount[1] = 2;
    character.itemCount[2] = 3;
    character.itemCount[3] = 4;
    HOJY_CHECK_EQ(hojy::mem::compactCarryItemSlots(character, 2), true);
    HOJY_CHECK_EQ(character.item[1], 202);
    HOJY_CHECK_EQ(character.item[2], 204);
    HOJY_CHECK_EQ(character.itemCount[2], 4);
    HOJY_CHECK_EQ(character.item[3], -1);
}

}

int main() {
    try {
        testSelectionKeepsInputOrderForEachResourceKind();
        testSelectionReturnsEmptyWhenNoQualifyingItemExists();
        testBagKeepsSaveSlotOrderForBattleScans();
        testCarrySlotCompactionCopiesWholeSlots();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
