#pragma once

#include <map>
#include <string>
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

bool buildTalkPageModel(const std::wstring &source,
                        int maximumLineWidth,
                        int linesPerPage,
                        const std::map<wchar_t, int> &glyphAdvances,
                        TalkPageModel &model);

bool buildTextBlockLayout(const std::vector<std::wstring> &sourceLines,
                          int maximumLineWidth,
                          int rowHeight,
                          int lineSpacing,
                          int border,
                          const std::map<wchar_t, int> &glyphAdvances,
                          TextBlockLayout &layout);

}
