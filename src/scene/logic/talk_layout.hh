#pragma once

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace hojy::scene::logic {

struct TalkPageModel final {
    std::vector<std::wstring> lines;
    int linesPerPage = 0;
};

struct TextBlockLayout final {
    std::vector<std::wstring> lines;
    int width = 0;
    int height = 0;
};

using GlyphPair = std::pair<wchar_t, wchar_t>;

struct TextMetricRequest final {
    std::set<wchar_t> characters;
    std::set<GlyphPair> pairs;
};

struct TextMetricsSnapshot final {
    std::map<wchar_t, int> advances;
    std::map<GlyphPair, int> pairAdjustments;
};

TextMetricRequest collectTalkMetricRequest(const std::wstring &source);
TextMetricRequest collectTextMetricRequest(
    const std::vector<std::wstring> &sourceLines);

bool buildTalkPageModel(const std::wstring &source,
                        int maximumLineWidth,
                        int linesPerPage,
                        const TextMetricsSnapshot &metrics,
                        TalkPageModel &model);

bool buildTextBlockLayout(const std::vector<std::wstring> &sourceLines,
                          int maximumLineWidth,
                          int rowHeight,
                          int lineSpacing,
                          int border,
                          const TextMetricsSnapshot &metrics,
                          TextBlockLayout &layout);

}
