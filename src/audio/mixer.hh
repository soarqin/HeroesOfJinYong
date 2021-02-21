#pragma once

#include <cstdint>

namespace hojy::audio {

class Mixer {
public:
    Mixer();

private:
    std::uint32_t audioDevice_ = 0;
    std::uint32_t sampleRate_ = 0;
};

}
