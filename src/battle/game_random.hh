#pragma once

#include "random.hh"

namespace hojy::battle {

class GameRandom final: public RandomSource {
public:
    int next(int upperExclusive) override;
    int next(int minimum, int maximum) override;
};

}
