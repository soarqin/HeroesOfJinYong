#include "scene/title_snapshot_builder.hh"

#include "test_support.hh"
#include "world/character.hh"

#include <iostream>

namespace {

void testPreviewSnapshotOwnsFormattedValuesAndColors() {
    hojy::world::state::CharacterData character{};
    character.maxMp = 50;
    character.mpType = 1;
    character.attack = 31;
    character.speed = 28;
    character.defence = 27;
    character.maxHp = 49;
    character.medic = 25;
    character.poison = 26;
    character.depoison = 27;
    character.fist = 28;
    character.sword = 29;
    character.blade = 30;
    character.special = 25;
    character.potential = 99;

    const auto snapshot = hojy::scene::buildTitlePreviewSnapshot(
        L"令狐冲", character, true, 6, 1, 42);
    HOJY_CHECK_EQ(snapshot.generation, 42ULL);
    HOJY_CHECK_EQ(snapshot.confirmationIndex, 1);
    HOJY_CHECK_EQ(snapshot.windowBorder, 6);
    HOJY_CHECK_EQ(snapshot.choices.size(), 2U);
    HOJY_CHECK_EQ(snapshot.properties.size(), 13U);
    HOJY_CHECK_EQ(snapshot.properties.at(0).row, 0);
    HOJY_CHECK_EQ(snapshot.properties.at(0).column, 0);
    HOJY_CHECK_EQ(snapshot.properties.at(0).highlighted, true);
    HOJY_CHECK_EQ(snapshot.properties.at(1).highlighted, true);

    character.maxMp = 1;
    character.attack = 1;
    HOJY_CHECK_EQ(snapshot.properties.at(0).value, 50);
    HOJY_CHECK_EQ(snapshot.properties.at(1).value, 31);
}

void testNameEntrySnapshotOwnsDisplayText() {
    auto snapshot = hojy::scene::buildTitleNameEntrySnapshot(L"张三");
    HOJY_CHECK_EQ(snapshot.name, L"张三");
    HOJY_CHECK_EQ(snapshot.displayText.find(L"张三") != std::wstring::npos,
                  true);
}

}

int main() {
    try {
        testPreviewSnapshotOwnsFormattedValuesAndColors();
        testNameEntrySnapshotOwnsDisplayText();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
