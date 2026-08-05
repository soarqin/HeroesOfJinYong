#include "talk_layout.hh"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <utility>

namespace hojy::scene::logic {
namespace {

bool appendWrappedLine(std::wstring line, int maximumLineWidth,
                       const std::map<wchar_t, int> &glyphAdvances,
                       std::vector<std::wstring> &output) {
    const auto length = line.length();
    if (length == 0) { return true; }
    std::size_t begin = 0;
    std::size_t lineWidth = 0;
    for (std::size_t index = 0; index < length; ++index) {
        auto &ch = line[index];
        if (ch == L'．') {
            if ((index > 0 && line[index - 1] == L'…')
                || (index + 1 < length && line[index + 1] == L'．')) {
                ch = L'…';
            } else {
                ch = L'。';
            }
        }
        int advance = 0;
        if (!(ch < 32 || ch > 0 && ch < 17)) {
            const auto found = glyphAdvances.find(ch);
            if (found == glyphAdvances.end() || found->second < 0) {
                return false;
            }
            advance = found->second;
        }
        if (lineWidth > std::numeric_limits<std::size_t>::max()
                - static_cast<std::size_t>(advance)) {
            return false;
        }
        lineWidth += static_cast<std::size_t>(advance);
        if (lineWidth > static_cast<std::size_t>(maximumLineWidth)) {
            output.emplace_back(line.substr(begin, index - begin));
            begin = index;
            lineWidth = static_cast<std::size_t>(advance);
        }
    }
    if (begin < length) { output.emplace_back(line.substr(begin)); }
    return true;
}

}

bool buildTalkPageModel(const std::wstring &source,
                        int maximumLineWidth,
                        int linesPerPage,
                        const std::map<wchar_t, int> &glyphAdvances,
                        TalkPageModel &model) {
    if (maximumLineWidth <= 0 || linesPerPage <= 0) { return false; }

    std::vector<std::wstring> logicalLines;
    std::wstring line;
    std::size_t index = 0;
    while (index < source.length()) {
        const auto separator = source.find(L'*', index);
        if (separator == std::wstring::npos) {
            line.append(source.substr(index));
            while (!line.empty() && line.back() == 0) { line.pop_back(); }
            if (!line.empty()) { logicalLines.emplace_back(std::move(line)); }
            break;
        }
        const auto length = separator - index;
        auto segment = source.substr(index, length);
        while (!segment.empty() && segment.back() == 12288) { segment.pop_back(); }
        while (!segment.empty() && segment.front() == 12288) {
            segment.erase(segment.begin());
        }
        line.append(segment);
        if (!line.empty() && (length < 12
            || (line.back() == L'．' && line.size() >= 2
                && *(line.end() - 2) != L'．'
                && (index + 2 >= source.length()
                    || source[index + 1] != L'．')))) {
            logicalLines.emplace_back(std::move(line));
            line.clear();
        }
        index = separator + 1;
        while (index < source.length() && source[index] == 12288) { ++index; }
    }

    TalkPageModel candidate;
    for (auto &logicalLine: logicalLines) {
        if (!appendWrappedLine(
                std::move(logicalLine), maximumLineWidth,
                glyphAdvances, candidate.lines)) {
            return false;
        }
    }
    candidate.linesPerPage = std::min(
        linesPerPage, static_cast<int>(candidate.lines.size()));
    model = std::move(candidate);
    return true;
}

bool buildTextBlockLayout(const std::vector<std::wstring> &sourceLines,
                          int maximumLineWidth,
                          int rowHeight,
                          int lineSpacing,
                          int border,
                          const std::map<wchar_t, int> &glyphAdvances,
                          TextBlockLayout &layout) {
    if (maximumLineWidth < 0 || rowHeight <= 0 || lineSpacing < 0
        || border < 0 || lineSpacing >= rowHeight) {
        return false;
    }
    TextBlockLayout candidate;
    int maximumContentWidth = 0;
    for (const auto &sourceLine: sourceLines) {
        std::size_t begin = 0;
        int lineWidth = 0;
        for (std::size_t index = 0; index < sourceLine.size(); ++index) {
            const auto ch = sourceLine[index];
            int advance = 0;
            if (!(ch < 32 || ch > 0 && ch < 17)) {
                const auto found = glyphAdvances.find(ch);
                if (found == glyphAdvances.end() || found->second < 0
                    || lineWidth > std::numeric_limits<int>::max()
                        - found->second) {
                    return false;
                }
                advance = found->second;
            }
            lineWidth += advance;
            if (lineWidth > maximumLineWidth) {
                maximumContentWidth = std::max(
                    maximumContentWidth, lineWidth - advance);
                candidate.lines.emplace_back(
                    sourceLine.substr(begin, index - begin));
                begin = index;
                lineWidth = advance;
            }
        }
        if (begin < sourceLine.size()) {
            maximumContentWidth = std::max(maximumContentWidth, lineWidth);
            candidate.lines.emplace_back(sourceLine.substr(begin));
        }
    }
    const auto width = static_cast<long long>(maximumContentWidth)
        + static_cast<long long>(border) * 2;
    const auto height = static_cast<long long>(rowHeight)
        * static_cast<long long>(candidate.lines.size())
        + static_cast<long long>(border) * 2 - lineSpacing;
    if (width < 0 || width > std::numeric_limits<int>::max()
        || height < 0 || height > std::numeric_limits<int>::max()) {
        return false;
    }
    candidate.width = static_cast<int>(width);
    candidate.height = static_cast<int>(height);
    layout = std::move(candidate);
    return true;
}

}
