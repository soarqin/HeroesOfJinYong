#include "game_random.hh"

#include "util/random.hh"

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
    return static_cast<int>(util::gRandom(minimum, maximum));
}

}
