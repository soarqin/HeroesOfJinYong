#include "rate_scheduler.hh"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace hojy::app {

namespace {

constexpr long double RateScale = 1000000.0L;

std::uint64_t toRateUnits(double rate) {
    if (!std::isfinite(rate) || rate < 0.0) {
        throw std::invalid_argument("rate must be finite and non-negative");
    }
    const auto scaled = static_cast<long double>(rate) * RateScale;
    if (scaled > static_cast<long double>(std::numeric_limits<std::uint64_t>::max())) {
        throw std::invalid_argument("rate is too large");
    }
    return static_cast<std::uint64_t>(std::llround(scaled));
}

}

RateScheduler::RateScheduler(double fixedRateHz, double targetRateHz):
    fixedRateUnits_(toRateUnits(fixedRateHz)),
    targetRateUnits_(toRateUnits(targetRateHz)) {
    if (fixedRateUnits_ == 0) {
        throw std::invalid_argument("fixed rate must be positive");
    }
}

std::uint32_t RateScheduler::advance() {
    if (targetRateUnits_ == 0) { return 0; }
    if (budgetUnits_ > std::numeric_limits<std::uint64_t>::max() - targetRateUnits_) {
        budgetUnits_ = fixedRateUnits_ - 1;
        return 0;
    }
    budgetUnits_ += targetRateUnits_;
    const auto ticks64 = budgetUnits_ / fixedRateUnits_;
    budgetUnits_ %= fixedRateUnits_;
    return static_cast<std::uint32_t>(std::min<std::uint64_t>(
        ticks64, std::numeric_limits<std::uint32_t>::max()));
}

}
