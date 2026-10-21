#pragma once

#include <cstdint>
#include <tuple>

namespace hojy::world::state {

[[nodiscard]] std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>
calcColorForMpType(std::int16_t type);

}
