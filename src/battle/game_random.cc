#include "game_random.hh"

#include "util/random.hh"

#include <stdexcept>

namespace hojy::battle {

int GameRandom::next(int upperExclusive) {
    if (upperExclusive <= 0) {
        throw std::invalid_argument("upperExclusive must be positive");
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
