#include "scene/logic/font_metrics.hh"
#include "test_support.hh"

#include <cstdint>
#include <iostream>

namespace {

void testIbmPlexMetricsCenterTheBaselineInTheLineBox() {
    std::int64_t glyphTop = 0;
    HOJY_CHECK_EQ(hojy::scene::logic::centeredGlyphTop(
                      26, 22.88, -3.12, -22, glyphTop), true);
    HOJY_CHECK_EQ(glyphTop, 1);
}

void testTallCjkMetricsUseAscenderAndDescenderTogether() {
    std::int64_t glyphTop = 0;
    HOJY_CHECK_EQ(hojy::scene::logic::centeredGlyphTop(
                      26, 30.16, -8.32, -22, glyphTop), true);
    HOJY_CHECK_EQ(glyphTop, 2);
}

void testInvalidVerticalMetricsAreRejected() {
    std::int64_t glyphTop = 7;
    HOJY_CHECK_EQ(hojy::scene::logic::centeredGlyphTop(
                      26, -4.0, 8.0, -10, glyphTop), false);
    HOJY_CHECK_EQ(glyphTop, 7);
}

}

int main() {
    try {
        testIbmPlexMetricsCenterTheBaselineInTheLineBox();
        testTallCjkMetricsUseAscenderAndDescenderTogether();
        testInvalidVerticalMetricsAreRejected();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
