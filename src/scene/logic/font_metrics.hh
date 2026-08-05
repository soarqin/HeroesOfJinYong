#pragma once

#include <cmath>
#include <cstdint>
#include <limits>

namespace hojy::scene::logic {

inline bool centeredGlyphTop(int fontSize,
                             double scaledAscender,
                             double scaledDescender,
                             std::int64_t glyphTopFromBaseline,
                             std::int64_t &result) noexcept {
    if (fontSize <= 0 || !std::isfinite(scaledAscender)
        || !std::isfinite(scaledDescender)
        || scaledAscender < scaledDescender) {
        return false;
    }
    const auto baselineValue = (static_cast<long double>(fontSize)
                                + static_cast<long double>(scaledAscender)
                                + static_cast<long double>(scaledDescender)) / 2.0L;
    if (!std::isfinite(baselineValue)
        || baselineValue <= static_cast<long double>(std::numeric_limits<std::int64_t>::min())
        || baselineValue >= static_cast<long double>(std::numeric_limits<std::int64_t>::max())) {
        return false;
    }
    const auto resultValue = std::round(baselineValue)
        + static_cast<long double>(glyphTopFromBaseline);
    if (!std::isfinite(resultValue)
        || resultValue < static_cast<long double>(std::numeric_limits<std::int64_t>::min())
        || resultValue > static_cast<long double>(std::numeric_limits<std::int64_t>::max())) {
        return false;
    }
    result = static_cast<std::int64_t>(resultValue);
    return true;
}

}
