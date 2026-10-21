#include "scene/logic/title_input_mode.hh"

#include "test_support.hh"

#include <iostream>
#include <memory>
#include <string>

namespace {

class RecordingContext final : public hojy::scene::TitleInputExecutionContext {
public:
    void executeMoveSelection(int delta) override { moveDelta = delta; }
    void executeActivateMainSelection() override { ++activateMainCount; }
    void executeActivateLoadSelection() override { ++activateLoadCount; }
    void executeReturnToMainMenu() override { ++returnCount; }
    void executeAppendName(std::wstring text) override {
        appendedText = std::move(text);
    }
    void executeEraseName() override { ++eraseCount; }
    void executeSubmitName() override { ++submitNameCount; }
    void executeSelectConfirmation(int index) override {
        confirmationIndex = index;
    }
    void executeActivateConfirmation() override {
        ++activateConfirmationCount;
    }
    void executeRerollCandidate() override { ++rerollCount; }

    int moveDelta = 0;
    int activateMainCount = 0;
    int activateLoadCount = 0;
    int returnCount = 0;
    std::wstring appendedText;
    int eraseCount = 0;
    int submitNameCount = 0;
    int confirmationIndex = -1;
    int activateConfirmationCount = 0;
    int rerollCount = 0;
};

void execute(std::unique_ptr<hojy::scene::TitleInputAction> action,
             RecordingContext &context) {
    if (action) { action->execute(context); }
}

void testMainAndLoadModesProduceDifferentActions() {
    RecordingContext context;
    hojy::scene::TitleMainMenuInputMode main;
    execute(main.keyAction(hojy::scene::InputKey::Up), context);
    HOJY_CHECK_EQ(context.moveDelta, -1);
    execute(main.keyAction(hojy::scene::InputKey::Accept), context);
    HOJY_CHECK_EQ(context.activateMainCount, 1);
    execute(main.keyAction(hojy::scene::InputKey::Cancel), context);
    HOJY_CHECK_EQ(context.returnCount, 0);

    hojy::scene::TitleLoadInputMode load;
    execute(load.keyAction(hojy::scene::InputKey::Down), context);
    HOJY_CHECK_EQ(context.moveDelta, 1);
    execute(load.keyAction(hojy::scene::InputKey::Space), context);
    HOJY_CHECK_EQ(context.activateLoadCount, 1);
    execute(load.keyAction(hojy::scene::InputKey::Cancel), context);
    HOJY_CHECK_EQ(context.returnCount, 1);
}

void testNameModeOwnsTextEditing() {
    RecordingContext context;
    hojy::scene::TitleNameInputMode mode;
    execute(mode.textAction(L"令狐"), context);
    HOJY_CHECK_EQ(context.appendedText, L"令狐");
    execute(mode.keyAction(hojy::scene::InputKey::Backspace), context);
    HOJY_CHECK_EQ(context.eraseCount, 1);
    execute(mode.keyAction(hojy::scene::InputKey::Accept), context);
    HOJY_CHECK_EQ(context.submitNameCount, 1);
    execute(mode.keyAction(hojy::scene::InputKey::Space), context);
    HOJY_CHECK_EQ(context.submitNameCount, 1);
    execute(mode.keyAction(hojy::scene::InputKey::Cancel), context);
    HOJY_CHECK_EQ(context.returnCount, 1);
}

void testConfirmationModeOwnsYesNoAndReroll() {
    RecordingContext context;
    hojy::scene::TitleConfirmationInputMode mode;
    execute(mode.keyAction(hojy::scene::InputKey::Left), context);
    HOJY_CHECK_EQ(context.confirmationIndex, 0);
    execute(mode.keyAction(hojy::scene::InputKey::Down), context);
    HOJY_CHECK_EQ(context.confirmationIndex, 1);
    execute(mode.keyAction(hojy::scene::InputKey::Accept), context);
    HOJY_CHECK_EQ(context.activateConfirmationCount, 1);
    execute(mode.keyAction(hojy::scene::InputKey::Cancel), context);
    HOJY_CHECK_EQ(context.rerollCount, 1);
}

}

int main() {
    try {
        testMainAndLoadModesProduceDifferentActions();
        testNameModeOwnsTextEditing();
        testConfirmationModeOwnsYesNoAndReroll();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
