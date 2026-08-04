#include "fixed_scheduler.hh"

#include <limits>
#include <stdexcept>

namespace hojy::app {

FixedTickAccumulator::FixedTickAccumulator(std::uint64_t tickMicros,
                                           std::uint32_t compatibilityDivisor,
                                           std::uint32_t maxCatchUpTicks):
    tickMicros_(tickMicros), compatibilityDivisor_(compatibilityDivisor),
    maxCatchUpTicks_(maxCatchUpTicks) {
    if (tickMicros_ == 0 || compatibilityDivisor_ == 0 || maxCatchUpTicks_ == 0) {
        throw std::invalid_argument("fixed tick parameters must be positive");
    }
}

TickBatch FixedTickAccumulator::advance(std::uint64_t elapsedMicros) {
    if (elapsedMicros > std::numeric_limits<std::uint64_t>::max() - remainderMicros_) {
        remainderMicros_ = tickMicros_ - 1;
        return {};
    }
    remainderMicros_ += elapsedMicros;
    const auto fixed64 = remainderMicros_ / tickMicros_;
    const auto fixed = static_cast<std::uint32_t>(
        fixed64 > maxCatchUpTicks_ ? maxCatchUpTicks_ : fixed64);
    if (fixed64 > maxCatchUpTicks_) {
        // Drop excess elapsed time deliberately. This keeps a suspended window
        // from executing an unbounded catch-up loop on the next frame.
        remainderMicros_ = 0;
        compatibilityRemainder_ = 0;
    } else {
        remainderMicros_ %= tickMicros_;
    }
    const auto compatibilityTotal =
        static_cast<std::uint64_t>(compatibilityRemainder_) + fixed;
    const auto compatibility = static_cast<std::uint32_t>(compatibilityTotal / compatibilityDivisor_);
    compatibilityRemainder_ = static_cast<std::uint32_t>(compatibilityTotal % compatibilityDivisor_);
    return {fixed, compatibility};
}

}
