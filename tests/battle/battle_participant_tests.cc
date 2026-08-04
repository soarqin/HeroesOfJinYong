#include "battle/battle_participant.hh"
#include "world/character.hh"
#include "test_support.hh"

#include <iostream>

int main() {
    try {
        hojy::world::state::CharacterData persistent{};
        persistent.id = 10;
        persistent.hp = 100;
        persistent.mp = 80;

        hojy::battle::BattleParticipant participant(persistent);
        participant.state().hp = 25;
        participant.state().mp = 10;
        HOJY_CHECK_EQ(persistent.hp, 100);
        HOJY_CHECK_EQ(participant.changed(), true);

        participant.discard();
        HOJY_CHECK_EQ(participant.state().hp, 100);
        HOJY_CHECK_EQ(participant.changed(), false);

        participant.state().hp = 40;
        participant.commit();
        HOJY_CHECK_EQ(persistent.hp, 40);
        HOJY_CHECK_EQ(participant.changed(), false);

        hojy::world::state::CharacterData enemy{};
        enemy.hp = 90;
        hojy::battle::BattleParticipant temporary(nullptr, enemy);
        temporary.state().hp = 10;
        hojy::world::state::CharacterData candidate = temporary.state();
        candidate.hp = 25;
        temporary.stageCommit(candidate);
        HOJY_CHECK_EQ(temporary.hasStagedCommit(), true);
        temporary.commit();
        HOJY_CHECK_EQ(temporary.state().hp, 25);
        HOJY_CHECK_EQ(enemy.hp, 90);
        temporary.state().hp = 1;
        temporary.discard();
        HOJY_CHECK_EQ(temporary.state().hp, 25);
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
