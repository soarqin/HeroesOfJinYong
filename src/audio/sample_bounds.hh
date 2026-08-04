#pragma once

#include <cstddef>
#include <limits>

namespace hojy::audio::detail {

inline bool checkedMidiSampleBytes(int sampleCount, std::size_t &bytes) noexcept {
    if (sampleCount < 0) { return false; }
    const auto count = static_cast<std::size_t>(sampleCount);
    if (count > std::numeric_limits<std::size_t>::max() / sizeof(short)) {
        return false;
    }
    bytes = count * sizeof(short);
    return true;
}

inline bool checkedAudioCvtLength(std::size_t bytes, int &length) noexcept {
    if (bytes > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        return false;
    }
    length = static_cast<int>(bytes);
    return true;
}

inline bool checkedAdlDataSize(std::size_t size, unsigned long &length) noexcept {
    if (size > static_cast<std::size_t>(
            std::numeric_limits<unsigned long>::max())) {
        return false;
    }
    length = static_cast<unsigned long>(size);
    return true;
}

}
