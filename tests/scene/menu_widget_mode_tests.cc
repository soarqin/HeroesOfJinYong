#include "scene/menu.hh"

#include "test_support.hh"

#include <iostream>
#include <memory>
#include <vector>

namespace {

class RecordingSink final: public hojy::scene::MenuSelectionSink {
public:
    void submit(hojy::scene::MenuSelection selection) override {
        selections.push_back(selection);
    }

    std::vector<hojy::scene::MenuSelection> selections;
};

void testOptionWidgetUsesOptionModeAfterVerticalConfiguration() {
    hojy::scene::MenuOption menu(
        static_cast<hojy::scene::Node *>(nullptr), 0, 0, 100, 100);
    menu.popup(hojy::scene::MenuEntries{
        {10, L"music", L"4", true},
        {20, L"sound", L"4", true},
    });
    HOJY_CHECK_EQ(menu.currIndex(), 0);
    menu.enableHorizonal(false);
    auto sink = std::make_shared<RecordingSink>();
    menu.setSelectionSink(sink);

    menu.consumeKeyIntent(hojy::scene::Node::KeyLeft);
    menu.applyInputLogic();

    HOJY_CHECK_EQ(sink->selections.size(), std::size_t(1));
    HOJY_CHECK_EQ(sink->selections.front().entryId, 10);
    HOJY_CHECK_EQ(sink->selections.front().gesture,
                  hojy::scene::MenuGesture::AdjustPrevious);
}

void testYesNoModeSurvivesLaterDirectionConfiguration() {
    hojy::scene::MenuYesNo menu(
        static_cast<hojy::scene::Node *>(nullptr), 0, 0, 100, 100);
    menu.popupWithYesNo();
    menu.enableHorizonal(false);
    auto sink = std::make_shared<RecordingSink>();
    menu.setSelectionSink(sink);

    menu.consumeKeyIntent(hojy::scene::Node::KeyUp);
    menu.applyInputLogic();
    menu.consumeKeyIntent(hojy::scene::Node::KeyOK);
    menu.applyInputLogic();

    HOJY_CHECK_EQ(sink->selections.size(), std::size_t(1));
    HOJY_CHECK_EQ(sink->selections.front().entryId, 0);
    HOJY_CHECK_EQ(sink->selections.front().gesture,
                  hojy::scene::MenuGesture::Activate);
}

void testYesNoModeSubmitsNoSelection() {
    hojy::scene::MenuYesNo menu(
        static_cast<hojy::scene::Node *>(nullptr), 0, 0, 100, 100);
    menu.popupWithYesNo();
    menu.enableHorizonal(true);
    auto sink = std::make_shared<RecordingSink>();
    menu.setSelectionSink(sink);

    HOJY_CHECK_EQ(menu.currIndex(), -1);
    menu.consumeKeyIntent(hojy::scene::Node::KeyOK);
    menu.applyInputLogic();
    HOJY_CHECK_EQ(sink->selections.empty(), true);

    menu.consumeKeyIntent(hojy::scene::Node::KeyRight);
    menu.applyInputLogic();
    menu.consumeKeyIntent(hojy::scene::Node::KeyOK);
    menu.applyInputLogic();

    HOJY_CHECK_EQ(sink->selections.size(), std::size_t(1));
    HOJY_CHECK_EQ(sink->selections.front().entryId, 1);
    HOJY_CHECK_EQ(sink->selections.front().gesture,
                  hojy::scene::MenuGesture::Activate);
}

}

int main() {
    try {
        testOptionWidgetUsesOptionModeAfterVerticalConfiguration();
        testYesNoModeSurvivesLaterDirectionConfiguration();
        testYesNoModeSubmitsNoSelection();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
