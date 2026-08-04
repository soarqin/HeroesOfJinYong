#include "game_random.hh"

#include "util/random.hh"

#include <cstdint>
#include <stdexcept>

namespace hojy::battle {

int GameRandom::next(int upperExclusive) {
    /* Original Z.DAT sub_3D612 accepts only bounds from 2 through 30000. */
    if (upperExclusive <= 1 || upperExclusive > OriginalRandomBoundMax) {
        return 0;
    }
    return static_cast<int>(util::gRandom(static_cast<util::Random::IntType>(upperExclusive)));
}

int GameRandom::next(int minimum, int maximum) {
    if (minimum > maximum) {
        throw std::invalid_argument("minimum must not exceed maximum");
    }
    const auto width = static_cast<std::int64_t>(maximum)
        - static_cast<std::int64_t>(minimum) + 1;
    const auto normalized = util::gRandom()
        % static_cast<util::Random::IntType>(width);
    return static_cast<int>(static_cast<std::int64_t>(minimum)
                            + static_cast<std::int64_t>(normalized));
}

}
