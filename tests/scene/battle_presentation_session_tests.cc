#include "scene/logic/presentation.hh"
#include "test_support.hh"

#include <iostream>

namespace {

void testInvalidationExpiresOldHandleAndToken() {
    hojy::scene::BattlePresentationSession session;
    const auto firstToken = session.begin();
    const auto firstHandle = session.handle();

    HOJY_CHECK_EQ(firstToken != 0, true);
    HOJY_CHECK_EQ(session.matches(firstToken), true);
    HOJY_CHECK_EQ(
        hojy::scene::BattlePresentationSession::matches(firstHandle, firstToken),
        true);

    session.invalidate();
    HOJY_CHECK_EQ(session.matches(firstToken), false);
    HOJY_CHECK_EQ(
        hojy::scene::BattlePresentationSession::matches(firstHandle, firstToken),
        false);

    const auto secondToken = session.begin();
    const auto secondHandle = session.handle();
    HOJY_CHECK_EQ(secondToken != firstToken, true);
    HOJY_CHECK_EQ(session.matches(firstToken), false);
    HOJY_CHECK_EQ(
        hojy::scene::BattlePresentationSession::matches(firstHandle, firstToken),
        false);
    HOJY_CHECK_EQ(
        hojy::scene::BattlePresentationSession::matches(secondHandle, secondToken),
        true);
}

void testBattlePresentationRequestsMustCarryConcreteStage() {
    HOJY_CHECK_EQ(
        hojy::scene::isConcreteBattlePresentationStage(
            hojy::scene::BattlePresentationStage::Any),
        false);
    HOJY_CHECK_EQ(
        hojy::scene::isConcreteBattlePresentationStage(
            hojy::scene::BattlePresentationStage::PlayerMenu),
        true);

    hojy::scene::BattleMenuRequest request{1};
    HOJY_CHECK_EQ(
        hojy::scene::isValidBattlePresentationRequest(
            request.sessionToken, request.actionGeneration,
            request.expectedStage),
        false);
    request.actionGeneration = 2;
    request.expectedStage = hojy::scene::BattlePresentationStage::PlayerMenu;
    HOJY_CHECK_EQ(
        hojy::scene::isValidBattlePresentationRequest(
            request.sessionToken, request.actionGeneration,
            request.expectedStage),
        true);
}

void testBattlePresentationRequestsCarryCompleteViewValues() {
    hojy::scene::BattleMenuRequest menu{1};
    menu.entries.push_back({8, L"休息"});
    menu.skills.push_back({2, L"野球拳"});
    HOJY_CHECK_EQ(menu.entries.at(0).actionId, 8);
    HOJY_CHECK_EQ(menu.skills.at(0).skillIndex, 2);

    hojy::scene::BattleItemSelectionRequest items{1};
    items.items.push_back({3, 4, L"药丸 x4", L"恢复体力", {}, {}, L"", L""});
    HOJY_CHECK_EQ(items.items.at(0).itemId, 3);
    HOJY_CHECK_EQ(items.items.at(0).count, 4);

    hojy::scene::BattleStatusSelectionRequest status{1};
    status.characters.rows.push_back({0, false, L"令狐冲", L" 10"});
    HOJY_CHECK_EQ(status.characters.rows.at(0).characterId, 0);
}

}

int main() {
    try {
        testInvalidationExpiresOldHandleAndToken();
        testBattlePresentationRequestsMustCarryConcreteStage();
        testBattlePresentationRequestsCarryCompleteViewValues();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
