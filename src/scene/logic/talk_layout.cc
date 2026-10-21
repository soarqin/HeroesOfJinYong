#include "talk_layout.hh"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <utility>

namespace hojy::scene::logic {
namespace {

bool isColorControl(wchar_t ch) {
    return ch > 0 && ch < 17;
}

bool isVisibleCharacter(wchar_t ch) {
    return ch >= 32;
}

void normalizeLegacyPunctuation(std::wstring &line) {
    for (std::size_t index = 0; index < line.size(); ++index) {
        auto &ch = line[index];
        if (ch != L'．') { continue; }
        if ((index > 0 && line[index - 1] == L'…')
            || (index + 1 < line.size() && line[index + 1] == L'．')) {
            ch = L'…';
        } else {
            ch = L'。';
        }
    }
}

void commitLogicalLine(std::wstring &line,
                       std::vector<std::wstring> &logicalLines) {
    normalizeLegacyPunctuation(line);
    logicalLines.emplace_back(std::move(line));
    line.clear();
}

std::vector<std::wstring> buildLogicalLines(const std::wstring &source) {
    std::vector<std::wstring> logicalLines;
    std::wstring line;
    std::size_t index = 0;
    while (index < source.length()) {
        const auto separator = source.find(L'*', index);
        if (separator == std::wstring::npos) {
            line.append(source.substr(index));
            while (!line.empty() && line.back() == 0) { line.pop_back(); }
            if (!line.empty()) { commitLogicalLine(line, logicalLines); }
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
            commitLogicalLine(line, logicalLines);
        }
        index = separator + 1;
        while (index < source.length() && source[index] == 12288) { ++index; }
    }
    return logicalLines;
}

void appendMetricRequest(const std::wstring &line, TextMetricRequest &request) {
    bool hasPrevious = false;
    wchar_t previous = 0;
    for (const auto ch: line) {
        if (isColorControl(ch)) { continue; }
        if (!isVisibleCharacter(ch)) {
            hasPrevious = false;
            continue;
        }
        request.characters.insert(ch);
        if (hasPrevious) { request.pairs.emplace(previous, ch); }
        previous = ch;
        hasPrevious = true;
    }
}

int pairAdjustment(const TextMetricsSnapshot &metrics,
                   wchar_t left, wchar_t right) {
    const auto found = metrics.pairAdjustments.find({left, right});
    return found == metrics.pairAdjustments.end() ? 0 : found->second;
}

bool appendWrappedLine(const std::wstring &line, int maximumLineWidth,
                       const TextMetricsSnapshot &metrics,
                       std::vector<std::wstring> &output) {
    const auto length = line.length();
    if (length == 0) { return true; }
    std::size_t begin = 0;
    std::int64_t lineWidth = 0;
    bool hasPrevious = false;
    wchar_t previous = 0;
    for (std::size_t index = 0; index < length; ++index) {
        const auto ch = line[index];
        int advance = 0;
        int adjustment = 0;
        if (isVisibleCharacter(ch)) {
            const auto found = metrics.advances.find(ch);
            if (found == metrics.advances.end() || found->second < 0) {
                return false;
            }
            advance = found->second;
            if (hasPrevious) {
                adjustment = pairAdjustment(metrics, previous, ch);
            }
        } else if (!isColorControl(ch)) {
            hasPrevious = false;
        }
        const auto step = static_cast<std::int64_t>(advance) + adjustment;
        if (step < 0 || lineWidth > std::numeric_limits<std::int64_t>::max() - step) {
            return false;
        }
        lineWidth += step;
        if (lineWidth > maximumLineWidth) {
            output.emplace_back(line.substr(begin, index - begin));
            begin = index;
            lineWidth = advance;
        }
        if (isVisibleCharacter(ch)) {
            previous = ch;
            hasPrevious = true;
        }
    }
    if (begin < length) { output.emplace_back(line.substr(begin)); }
    return true;
}

}

TextMetricRequest collectTalkMetricRequest(const std::wstring &source) {
    TextMetricRequest request;
    for (const auto &line: buildLogicalLines(source)) {
        appendMetricRequest(line, request);
    }
    return request;
}

TextMetricRequest collectTextMetricRequest(
        const std::vector<std::wstring> &sourceLines) {
    TextMetricRequest request;
    for (const auto &line: sourceLines) { appendMetricRequest(line, request); }
    return request;
}

bool buildTalkPageModel(const std::wstring &source,
                        int maximumLineWidth,
                        int linesPerPage,
                        const TextMetricsSnapshot &metrics,
                        TalkPageModel &model) {
    if (maximumLineWidth <= 0 || linesPerPage <= 0) { return false; }

    auto logicalLines = buildLogicalLines(source);

    TalkPageModel candidate;
    for (const auto &logicalLine: logicalLines) {
        if (!appendWrappedLine(
                logicalLine, maximumLineWidth, metrics, candidate.lines)) {
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
                          const TextMetricsSnapshot &metrics,
                          TextBlockLayout &layout) {
    if (maximumLineWidth < 0 || rowHeight <= 0 || lineSpacing < 0
        || border < 0 || lineSpacing >= rowHeight) {
        return false;
    }
    TextBlockLayout candidate;
    int maximumContentWidth = 0;
    for (const auto &sourceLine: sourceLines) {
        std::size_t begin = 0;
        std::int64_t lineWidth = 0;
        bool hasPrevious = false;
        wchar_t previous = 0;
        for (std::size_t index = 0; index < sourceLine.size(); ++index) {
            const auto ch = sourceLine[index];
            int advance = 0;
            int adjustment = 0;
            if (isVisibleCharacter(ch)) {
                const auto found = metrics.advances.find(ch);
                if (found == metrics.advances.end() || found->second < 0) {
                    return false;
                }
                advance = found->second;
                if (hasPrevious) {
                    adjustment = pairAdjustment(metrics, previous, ch);
                }
            } else if (!isColorControl(ch)) {
                hasPrevious = false;
            }
            const auto step = static_cast<std::int64_t>(advance) + adjustment;
            if (step < 0 || lineWidth > std::numeric_limits<std::int64_t>::max() - step) {
                return false;
            }
            lineWidth += step;
            if (lineWidth > maximumLineWidth) {
                maximumContentWidth = std::max(
                    maximumContentWidth, static_cast<int>(lineWidth - step));
                candidate.lines.emplace_back(
                    sourceLine.substr(begin, index - begin));
                begin = index;
                lineWidth = advance;
            }
            if (isVisibleCharacter(ch)) {
                previous = ch;
                hasPrevious = true;
            }
        }
        if (begin < sourceLine.size()) {
            if (lineWidth > std::numeric_limits<int>::max()) { return false; }
            maximumContentWidth = std::max(
                maximumContentWidth, static_cast<int>(lineWidth));
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
