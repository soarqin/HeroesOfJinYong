#pragma once

#include <cstdint>

namespace hojy::audio {

class Mixer final {
public:
    Mixer();
    ~Mixer();

    void pause(bool on) const;

private:
    std::uint32_t audioDevice_ = 0;
    std::uint32_t sampleRate_ = 0;
};

}
