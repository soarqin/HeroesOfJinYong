#include "scene/item_selection_controller.hh"

#include "test_support.hh"

#include <iostream>
#include <vector>

namespace {

class Host final : public hojy::scene::ItemSelectionHost {
public:
    void closeItemSelection() override { closed = true; }
    void replaceItemSelection(
            std::vector<hojy::scene::ItemViewEntrySnapshot> items) override {
        replacement = std::move(items);
    }
    void showCharacterSelection(
            hojy::scene::CharacterSelectionRequest) override {}
    void showItemMessage(hojy::scene::ItemMessageRequest) override {}
    void useQuestItem(std::int16_t) override {}

    bool closed = false;
    std::vector<hojy::scene::ItemViewEntrySnapshot> replacement;
};

void testFunctionControllerSupportsTheTypedBindContract() {
    bool selected = false;
    bool cancelled = false;
    hojy::scene::FunctionItemSelectionController controller(
        [&selected](hojy::scene::ItemSelectionHost &, std::int16_t itemId) {
            selected = itemId == 17;
        },
        [&cancelled](hojy::scene::ItemSelectionHost &) {
            cancelled = true;
        });
    const std::vector<hojy::scene::ItemViewEntrySnapshot> items = {
        {17, 1, L"item", L"description", {}, {}, L"", L""},
    };
    controller.bindItems(items);

    Host host;
    controller.select(host, 17);
    controller.cancel(host);
    HOJY_CHECK_EQ(selected, true);
    HOJY_CHECK_EQ(cancelled, true);
}

}

int main() {
    try {
        testFunctionControllerSupportsTheTypedBindContract();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
