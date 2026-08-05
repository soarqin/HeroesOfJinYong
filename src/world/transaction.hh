#pragma once

#include <cstdint>

namespace hojy::world::state {

[[nodiscard]] std::uint64_t stateRevision() noexcept;
void bumpStateRevision() noexcept;

}
