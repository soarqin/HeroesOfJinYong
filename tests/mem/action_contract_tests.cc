#include "battle/formulas.hh"
#include "data/factors.hh"
#include "mem/action.hh"
#include "mem/bag.hh"
#include "mem/savedata.hh"
#include "mem/strings.hh"
#include "test_support.hh"

#include <cstring>
#include <iostream>
#include <string>

namespace hojy::data {
Factors gFactors{};
}

namespace hojy::mem {
SaveData gSaveData{};
Bag gBag{};
Strings gStrings{};

bool Bag::remove(std::int16_t, std::int16_t) {
    return false;
}
}

namespace {

template<typename T, typename Container>
void loadRecords(Container &container, const T *records, std::size_t count) {
    const auto *bytes = reinterpret_cast<const char *>(records);
    HOJY_CHECK_EQ(container.deserialize(
        std::string(bytes, bytes + sizeof(T) * count)), true);
}

void prepareData() {
    hojy::mem::ItemData items[2]{};
    items[0].id = 0;
    items[0].skillId = -1;
    items[0].reqExp = 100;
    items[0].reqExpForMakeItem = 100;
    items[1].id = 1;
    items[1].skillId = 1;
    items[1].reqExp = 100;
    items[1].reqExpForMakeItem = 100;
    loadRecords(hojy::mem::gSaveData.itemInfo, items, 2);

    hojy::mem::SkillData skills[2]{};
    skills[1].id = 1;
    skills[1].reqMp = 10;
    skills[1].damage[5] = 60;
    loadRecords(hojy::mem::gSaveData.skillInfo, skills, 2);
}

hojy::mem::CharacterData makeCharacter() {
    hojy::mem::CharacterData character{};
    character.equip[0] = character.equip[1] = -1;
    character.hp = character.maxHp = 999;
    character.mp = character.maxMp = 100;
    character.stamina = 100;
    character.attack = 80;
    character.defence = 10;
    character.level = 5;
    character.potential = 90;
    character.skillId[0] = 1;
    character.skillLevel[0] = 500;
    return character;
}

void experienceRequirementsUseOriginalPotentialTier() {
    HOJY_CHECK_EQ(hojy::mem::getExpForSkillLearn(1, 0, 0), 700);
    HOJY_CHECK_EQ(hojy::mem::getExpForSkillLearn(1, 1, 0), 1400);
    HOJY_CHECK_EQ(hojy::mem::getExpForSkillLearn(0, 0, 0), 1400);
    HOJY_CHECK_EQ(hojy::mem::getExpForSkillLearn(1, 9, 0), 0);
    HOJY_CHECK_EQ(hojy::mem::getExpForMakeItem(1, 0), 700);
    HOJY_CHECK_EQ(hojy::mem::getExpForMakeItem(1, 90), 100);
}

void levelExperienceRejectsNonPositiveLevels() {
    HOJY_CHECK_EQ(hojy::mem::getExpForLevelUp(0), 0);
    HOJY_CHECK_EQ(hojy::mem::getExpForLevelUp(-1), 0);
}

void skillBookUsesItsOwnPropertyContract() {
    auto character = makeCharacter();
    character.hp = 50;
    character.poisoned = 20;
    character.stamina = 30;
    character.mp = 40;
    character.mpType = 0;
    character.doubleAttack = 0;
    character.attack = 95;

    hojy::mem::ItemData book{};
    book.addHp = 100;
    book.addPoisoned = -20;
    book.addStamina = 50;
    book.addMp = 100;
    book.addMaxHp = 10;
    book.addMaxMp = 20;
    book.changeMpType = 2;
    book.addAttack = 20;
    book.addDoubleAttack = 1;

    hojy::mem::applyBookChanges(&character, &book);

    HOJY_CHECK_EQ(character.hp, 50);
    HOJY_CHECK_EQ(character.poisoned, 20);
    HOJY_CHECK_EQ(character.stamina, 30);
    HOJY_CHECK_EQ(character.mp, 40);
    HOJY_CHECK_EQ(character.maxHp, 999);
    HOJY_CHECK_EQ(character.maxMp, 120);
    HOJY_CHECK_EQ(character.mpType, 2);
    HOJY_CHECK_EQ(character.attack, 100);
    HOJY_CHECK_EQ(character.doubleAttack, 1);
}

void postDamageChargesOnceAndKeepsStoredProgress() {
    auto character = makeCharacter();
    character.mp = 100;
    character.stamina = 20;
    character.skillLevel[0] = 998;
    bool levelUp = false;

    HOJY_CHECK_EQ(hojy::mem::calcSkillMpCost(hojy::mem::gSaveData.skillInfo[1], 5), 30);
    hojy::mem::postDamage(&character, 0, 5, 3, levelUp);

    HOJY_CHECK_EQ(character.skillLevel[0], 999);
    HOJY_CHECK_EQ(character.mp, 70);
    HOJY_CHECK_EQ(character.stamina, 17);
}

void damageReportsOriginalExperience() {
    auto attacker = makeCharacter();
    auto defender = makeCharacter();
    std::int16_t damage = 0;
    std::int16_t poisoned = 0;
    std::int16_t experience = 0;
    bool dead = false;

    HOJY_CHECK_EQ(hojy::mem::actDamage(
        &attacker, &defender, 0, 0, 1, 0, 5,
        damage, poisoned, experience, dead), true);
    HOJY_CHECK_EQ(experience, damage / 5);
    HOJY_CHECK_EQ(dead, false);
}

void roundEndAndLevelUpUseBatchContracts() {
    auto character = makeCharacter();
    character.hp = 10;
    character.hurt = 40;
    character.poisoned = 20;
    HOJY_CHECK_EQ(hojy::mem::actRoundEndDrain(&character, false), -4);
    HOJY_CHECK_EQ(character.hp, 6);

    character.hp = 4;
    HOJY_CHECK_EQ(hojy::mem::actRoundEndDrain(&character, false), -3);
    HOJY_CHECK_EQ(character.hp, 1);

    character.special = 30;
    character.throwing = 0;
    character.maxHp = character.hp = 100;
    character.hpAddOnLevelUp = 1;
    const auto oldMaxHp = character.maxHp;
    hojy::mem::actLevelup(&character, 2);
    HOJY_CHECK_EQ(character.level, 7);
    HOJY_CHECK_EQ(character.special, 30);
    HOJY_CHECK_EQ(character.hp, character.maxHp);
    if (!(character.maxHp > oldMaxHp)) {
        throw std::runtime_error("batch level-up must increase max hp");
    }
}

}

int main() {
    try {
        prepareData();
        experienceRequirementsUseOriginalPotentialTier();
        levelExperienceRejectsNonPositiveLevels();
        skillBookUsesItsOwnPropertyContract();
        postDamageChargesOnceAndKeepsStoredProgress();
        damageReportsOriginalExperience();
        roundEndAndLevelUpUseBatchContracts();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
