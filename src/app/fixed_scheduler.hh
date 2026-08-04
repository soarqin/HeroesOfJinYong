#pragma once

#include <cstdint>

namespace hojy::app {

struct TickBatch {
    std::uint32_t fixedTicks = 0;
    std::uint32_t compatibilityTicks = 0;
};

class FixedTickAccumulator final {
public:
    explicit FixedTickAccumulator(std::uint64_t tickMicros,
                                   std::uint32_t compatibilityDivisor = 4,
                                   std::uint32_t maxCatchUpTicks = 8);

    [[nodiscard]] TickBatch advance(std::uint64_t elapsedMicros);
    [[nodiscard]] std::uint64_t remainderMicros() const { return remainderMicros_; }

private:
    std::uint64_t tickMicros_;
    std::uint32_t compatibilityDivisor_;
    std::uint32_t maxCatchUpTicks_;
    std::uint64_t remainderMicros_ = 0;
    std::uint32_t compatibilityRemainder_ = 0;
};

}
