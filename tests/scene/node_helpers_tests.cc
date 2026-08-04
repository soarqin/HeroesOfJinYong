#include "scene/event_helpers.hh"
#include "scene/node_helpers.hh"
#include "test_support.hh"

#include <iostream>

namespace {

struct FakePanel {
    int renderCount = 0;
};

void testInvokeIfPresentSkipsNullAndInvokesLivePanel() {
    FakePanel panel;
    hojy::scene::detail::invokeIfPresent(
        &panel, [](FakePanel &value) { ++value.renderCount; });
    HOJY_CHECK_EQ(panel.renderCount, 1);

    FakePanel *missing = nullptr;
    bool called = false;
    hojy::scene::detail::invokeIfPresent(
        missing, [&called](FakePanel &) { called = true; });
    HOJY_CHECK_EQ(called, false);
}

void testDeferredBranchStartAppliesFailureImmediately() {
    std::size_t index = 10;
    std::size_t successAdvance = 3;
    std::size_t failureAdvance = 5;

    const auto paused = hojy::scene::detail::resolveDeferredBranchStart(
        false, index, successAdvance, failureAdvance);

    HOJY_CHECK_EQ(paused, false);
    HOJY_CHECK_EQ(index, 15U);
    HOJY_CHECK_EQ(successAdvance, 0U);
    HOJY_CHECK_EQ(failureAdvance, 0U);
}

void testDeferredBranchStartKeepsPendingAdvanceWhenStarted() {
    std::size_t index = 10;
    std::size_t successAdvance = 3;
    std::size_t failureAdvance = 5;

    const auto paused = hojy::scene::detail::resolveDeferredBranchStart(
        true, index, successAdvance, failureAdvance);

    HOJY_CHECK_EQ(paused, true);
    HOJY_CHECK_EQ(index, 10U);
    HOJY_CHECK_EQ(successAdvance, 3U);
    HOJY_CHECK_EQ(failureAdvance, 5U);
}

}

int main() {
    try {
        testInvokeIfPresentSkipsNullAndInvokesLivePanel();
        testDeferredBranchStartAppliesFailureImmediately();
        testDeferredBranchStartKeepsPendingAdvanceWhenStarted();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
