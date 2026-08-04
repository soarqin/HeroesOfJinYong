#pragma once

#include <cstdint>

namespace hojy::app {

class RateScheduler final {
public:
    RateScheduler(double fixedRateHz, double targetRateHz);

    [[nodiscard]] std::uint32_t advance();

private:
    std::uint64_t fixedRateUnits_;
    std::uint64_t targetRateUnits_;
    std::uint64_t budgetUnits_ = 0;
};

}
