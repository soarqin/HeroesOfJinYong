#include "scene/logic/talk_layout.hh"
#include "test_support.hh"

#include <iostream>
#include <map>

namespace {

void testLayoutBuildsImmutablePagesFromValueMetrics() {
    const std::map<wchar_t, int> advances{
        {L'甲', 5}, {L'乙', 5}, {L'丙', 5}, {L'丁', 5},
    };
    hojy::scene::logic::TalkPageModel model;

    HOJY_CHECK_EQ(hojy::scene::logic::buildTalkPageModel(
                      L"甲乙丙丁", 10, 1, advances, model), true);
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
    const std::map<wchar_t, int> advances{{L'甲', 5}};

    HOJY_CHECK_EQ(hojy::scene::logic::buildTalkPageModel(
                      L"甲", 0, 1, advances, model), false);
    HOJY_CHECK_EQ(model.lines, before.lines);
    HOJY_CHECK_EQ(model.linesPerPage, before.linesPerPage);
}

void testLayoutNormalizesLegacySeparatorsAndEllipsis() {
    const std::map<wchar_t, int> advances{
        {L'甲', 5}, {L'乙', 5}, {L'。', 5}, {L'…', 5},
    };
    hojy::scene::logic::TalkPageModel model;

    HOJY_CHECK_EQ(hojy::scene::logic::buildTalkPageModel(
                      L"甲*乙．", 20, 2, advances, model), true);
    HOJY_CHECK_EQ(model.lines.size(), 2U);
    HOJY_CHECK_EQ(model.lines[0], L"甲");
    HOJY_CHECK_EQ(model.lines[1], L"乙。");
}

void testTextBlockLayoutIsReadyBeforeRenderPreparation() {
    const std::map<wchar_t, int> advances{
        {L'甲', 5}, {L'乙', 5}, {L'丙', 5},
    };
    hojy::scene::logic::TextBlockLayout layout;

    HOJY_CHECK_EQ(hojy::scene::logic::buildTextBlockLayout(
                      {L"甲乙丙"}, 10, 8, 2, 2, advances, layout), true);
    HOJY_CHECK_EQ(layout.lines.size(), 2U);
    HOJY_CHECK_EQ(layout.lines[0], L"甲乙");
    HOJY_CHECK_EQ(layout.lines[1], L"丙");
    HOJY_CHECK_EQ(layout.width, 14);
    HOJY_CHECK_EQ(layout.height, 18);
}

}

int main() {
    try {
        testLayoutBuildsImmutablePagesFromValueMetrics();
        testLayoutFailurePreservesPreviousModel();
        testLayoutNormalizesLegacySeparatorsAndEllipsis();
        testTextBlockLayoutIsReadyBeforeRenderPreparation();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
