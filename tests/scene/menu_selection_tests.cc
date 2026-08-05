#include "scene/logic/menu.hh"

#include "test_support.hh"

#include <iostream>
#include <vector>

namespace {

class RecordingContext final : public hojy::scene::MenuInputExecutionContext {
public:
    int currentIndex() const noexcept override { return index; }
    int entryCount() const noexcept override {
        return static_cast<int>(ids.size());
    }
    bool checkboxEnabled() const noexcept override { return checkbox; }
    void moveSelection(int delta, bool wrap) override {
        if (ids.empty()) { return; }
        auto next = index + delta;
        if (wrap) {
            while (next < 0) { next += entryCount(); }
            while (next >= entryCount()) { next -= entryCount(); }
        }
        index = next < 0 || next >= entryCount() ? index : next;
    }
    void selectIndex(int value) override {
        if (value >= 0 && value < entryCount()) { index = value; }
    }
    std::int32_t entryIdAt(int value) const override {
        return value >= 0 && value < entryCount() ? ids[value] : -1;
    }
    void submit(hojy::scene::MenuGesture gesture) override {
        selections.push_back({++token, entryIdAt(index), gesture});
    }

    std::vector<std::int32_t> ids{10, 20, 30};
    std::vector<hojy::scene::MenuSelection> selections;
    int index = 0;
    bool checkbox = false;
    std::uint64_t token = 0;
};

void testVerticalModeEmitsStableEntryIds() {
    RecordingContext context;
    hojy::scene::VerticalMenuInputMode mode;
    auto action = mode.keyAction(hojy::scene::InputKey::Down);
    HOJY_CHECK_EQ(action != nullptr, true);
    action->execute(context);
    HOJY_CHECK_EQ(context.currentIndex(), 1);
    action = mode.keyAction(hojy::scene::InputKey::Accept);
    action->execute(context);
    HOJY_CHECK_EQ(context.selections.size(), std::size_t(1));
    HOJY_CHECK_EQ(context.selections[0].entryId, 20);
    HOJY_CHECK_EQ(context.selections[0].gesture, hojy::scene::MenuGesture::Activate);
}

void testOptionModeUsesTypedAdjustmentGestures() {
    RecordingContext context;
    hojy::scene::HorizontalOptionMenuInputMode mode;
    auto action = mode.keyAction(hojy::scene::InputKey::Left);
    action->execute(context);
    HOJY_CHECK_EQ(context.selections[0].gesture,
                  hojy::scene::MenuGesture::AdjustPrevious);
    HOJY_CHECK_EQ(context.selections[0].entryId, 10);
}

}

int main() {
    try {
        testVerticalModeEmitsStableEntryIds();
        testOptionModeUsesTypedAdjustmentGestures();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
