#include "scene/logic/talk_layout.hh"
#include "test_support.hh"

#include <iostream>
#include <map>

namespace {

void testLayoutBuildsImmutablePagesFromValueMetrics() {
    hojy::scene::logic::TextMetricsSnapshot metrics;
    metrics.advances = {
        {L'甲', 5}, {L'乙', 5}, {L'丙', 5}, {L'丁', 5}};
    hojy::scene::logic::TalkPageModel model;

    HOJY_CHECK_EQ(hojy::scene::logic::buildTalkPageModel(
                      L"甲乙丙丁", 10, 1, metrics, model), true);
    HOJY_CHECK_EQ(model.lines.size(), 2U);
    HOJY_CHECK_EQ(model.lines[0], L"甲乙");
    HOJY_CHECK_EQ(model.lines[1], L"丙丁");
    HOJY_CHECK_EQ(model.linesPerPage, 1);
}

void testLayoutFailurePreservesPreviousModel() {
    hojy::scene::logic::TalkPageModel model;
    model.lines = {L"旧页"};
    model.linesPerPage = 1;
    const auto before = model;
    hojy::scene::logic::TextMetricsSnapshot metrics;
    metrics.advances = {{L'甲', 5}};

    HOJY_CHECK_EQ(hojy::scene::logic::buildTalkPageModel(
                      L"甲", 0, 1, metrics, model), false);
    HOJY_CHECK_EQ(model.lines, before.lines);
    HOJY_CHECK_EQ(model.linesPerPage, before.linesPerPage);
}

void testLayoutNormalizesLegacySeparatorsAndEllipsis() {
    hojy::scene::logic::TextMetricsSnapshot metrics;
    metrics.advances = {
        {L'甲', 5}, {L'乙', 5}, {L'。', 5}, {L'…', 5}};
    hojy::scene::logic::TalkPageModel model;

    HOJY_CHECK_EQ(hojy::scene::logic::buildTalkPageModel(
                      L"甲*乙．", 20, 2, metrics, model), true);
    HOJY_CHECK_EQ(model.lines.size(), 2U);
    HOJY_CHECK_EQ(model.lines[0], L"甲");
    HOJY_CHECK_EQ(model.lines[1], L"乙。");
}

void testMissingGlyphKeepsLegacySeparatorsAndWrapping() {
    hojy::scene::logic::TextMetricsSnapshot metrics;
    metrics.advances = {
        {L'甲', 5}, {L'乙', 5}, {L'丙', 5}, {L'缺', 0}};
    hojy::scene::logic::TalkPageModel model;

    HOJY_CHECK_EQ(hojy::scene::logic::buildTalkPageModel(
                      L"甲乙*丙缺", 10, 2, metrics, model), true);
    HOJY_CHECK_EQ(model.lines.size(), 2U);
    HOJY_CHECK_EQ(model.lines[0], L"甲乙");
    HOJY_CHECK_EQ(model.lines[1], L"丙缺");
}

void testKerningIsAppliedBeforeTalkWrapping() {
    hojy::scene::logic::TextMetricsSnapshot metrics;
    metrics.advances = {{L'A', 10}, {L'V', 10}};
    metrics.pairAdjustments = {{{L'A', L'V'}, -3}};
    hojy::scene::logic::TalkPageModel model;

    HOJY_CHECK_EQ(hojy::scene::logic::buildTalkPageModel(
                      L"AV", 18, 2, metrics, model), true);
    HOJY_CHECK_EQ(model.lines.size(), 1U);
    HOJY_CHECK_EQ(model.lines[0], L"AV");
}

void testWrappedLineDoesNotKeepPreviousLineKerning() {
    hojy::scene::logic::TextMetricsSnapshot metrics;
    metrics.advances = {{L'A', 10}, {L'V', 10}};
    metrics.pairAdjustments = {{{L'A', L'V'}, 2}};
    hojy::scene::logic::TalkPageModel model;

    HOJY_CHECK_EQ(hojy::scene::logic::buildTalkPageModel(
                      L"AVV", 20, 2, metrics, model), true);
    HOJY_CHECK_EQ(model.lines.size(), 2U);
    HOJY_CHECK_EQ(model.lines[0], L"A");
    HOJY_CHECK_EQ(model.lines[1], L"VV");
}

void testMetricRequestUsesNormalizedDialogueText() {
    const auto request = hojy::scene::logic::collectTalkMetricRequest(
        L"甲乙乙乙乙乙乙乙乙乙乙乙．*丙");
    HOJY_CHECK_EQ(request.characters.count(L'．'), 0U);
    HOJY_CHECK_EQ(request.characters.count(L'。'), 1U);
    HOJY_CHECK_EQ(request.pairs.count({L'乙', L'。'}), 1U);
}

void testMetricRequestDoesNotKernAcrossDialogueLines() {
    const auto request = hojy::scene::logic::collectTalkMetricRequest(L"A*V");
    HOJY_CHECK_EQ(request.pairs.count({L'A', L'V'}), 0U);

    const auto colored = hojy::scene::logic::collectTextMetricRequest(
        {std::wstring(L"A") + wchar_t(2) + L"V"});
    HOJY_CHECK_EQ(colored.pairs.count({L'A', L'V'}), 1U);
}

void testTextBlockLayoutIsReadyBeforeRenderPreparation() {
    hojy::scene::logic::TextMetricsSnapshot metrics;
    metrics.advances = {{L'甲', 5}, {L'乙', 5}, {L'丙', 5}};
    hojy::scene::logic::TextBlockLayout layout;

    HOJY_CHECK_EQ(hojy::scene::logic::buildTextBlockLayout(
                      {L"甲乙丙"}, 10, 8, 2, 2, metrics, layout), true);
    HOJY_CHECK_EQ(layout.lines.size(), 2U);
    HOJY_CHECK_EQ(layout.lines[0], L"甲乙");
    HOJY_CHECK_EQ(layout.lines[1], L"丙");
    HOJY_CHECK_EQ(layout.width, 14);
    HOJY_CHECK_EQ(layout.height, 18);
}

void testTextBlockWidthMatchesRenderedKerning() {
    hojy::scene::logic::TextMetricsSnapshot metrics;
    metrics.advances = {{L'A', 10}, {L'V', 10}};
    metrics.pairAdjustments = {{{L'A', L'V'}, -3}};
    hojy::scene::logic::TextBlockLayout layout;

    HOJY_CHECK_EQ(hojy::scene::logic::buildTextBlockLayout(
                      {L"AV"}, 18, 8, 2, 2, metrics, layout), true);
    HOJY_CHECK_EQ(layout.lines.size(), 1U);
    HOJY_CHECK_EQ(layout.width, 21);
}

}

int main() {
    try {
        testLayoutBuildsImmutablePagesFromValueMetrics();
        testLayoutFailurePreservesPreviousModel();
        testLayoutNormalizesLegacySeparatorsAndEllipsis();
        testMissingGlyphKeepsLegacySeparatorsAndWrapping();
        testKerningIsAppliedBeforeTalkWrapping();
        testWrappedLineDoesNotKeepPreviousLineKerning();
        testMetricRequestUsesNormalizedDialogueText();
        testMetricRequestDoesNotKernAcrossDialogueLines();
        testTextBlockLayoutIsReadyBeforeRenderPreparation();
        testTextBlockWidthMatchesRenderedKerning();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
