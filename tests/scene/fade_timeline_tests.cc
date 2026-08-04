#include "scene/fade_timeline.hh"

#include "test_support.hh"

#include <iostream>

namespace {

void testFadeTimelineAdvancesOutsideRenderAndClampsAlpha() {
    hojy::scene::FadeTimeline fade(1000, 10, false);
    fade.advance(1000);
    HOJY_CHECK_EQ(fade.alpha(), std::uint8_t(0));
    HOJY_CHECK_EQ(fade.completed(), false);

    fade.advance(2270);
    HOJY_CHECK_EQ(fade.alpha(), std::uint8_t(127));
    HOJY_CHECK_EQ(fade.completed(), false);

    fade.advance(3560);
    HOJY_CHECK_EQ(fade.alpha(), std::uint8_t(255));
    HOJY_CHECK_EQ(fade.completed(), true);
}

void testFadeInReversesVisibleAlpha() {
    hojy::scene::FadeTimeline fade(0, 10, true);
    fade.advance(0);
    HOJY_CHECK_EQ(fade.alpha(), std::uint8_t(255));
    fade.advance(1000);
    HOJY_CHECK_EQ(fade.alpha(), std::uint8_t(155));
}

}

int main() {
    try {
        testFadeTimelineAdvancesOutsideRenderAndClampsAlpha();
        testFadeInReversesVisibleAlpha();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
