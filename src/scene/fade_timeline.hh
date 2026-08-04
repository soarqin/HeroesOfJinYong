#pragma once

#include <algorithm>
#include <cstdint>

namespace hojy::scene {

class FadeTimeline final {
public:
    FadeTimeline(std::uint64_t startMicros, std::uint64_t microsPerAlpha,
                 bool reverse):
        startMicros_(startMicros),
        microsPerAlpha_(std::max<std::uint64_t>(1, microsPerAlpha)),
        reverse_(reverse) {}

    void advance(std::uint64_t nowMicros) {
        const auto elapsed = nowMicros >= startMicros_ ? nowMicros - startMicros_ : 0;
        const auto raw = elapsed / microsPerAlpha_;
        completed_ = raw > 255;
        progress_ = static_cast<std::uint8_t>(std::min<std::uint64_t>(raw, 255));
    }

    [[nodiscard]] std::uint8_t alpha() const {
        return reverse_ ? static_cast<std::uint8_t>(255 - progress_) : progress_;
    }

    [[nodiscard]] bool completed() const { return completed_; }

private:
    std::uint64_t startMicros_;
    std::uint64_t microsPerAlpha_;
    std::uint8_t progress_ = 0;
    bool reverse_ = false;
    bool completed_ = false;
};

}
