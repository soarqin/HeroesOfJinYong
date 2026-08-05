#include "rle.hh"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>

namespace hojy::scene::logic {

bool validateRleData(const std::string &data) noexcept {
    if (data.size() < 8) { return false; }
    std::int16_t header[4]{};
    std::memcpy(header, data.data(), sizeof(header));
    const auto width = static_cast<int>(header[0]);
    const auto height = static_cast<int>(header[1]);
    if (width <= 0 || height <= 0) { return false; }

    std::size_t position = sizeof(header);
    for (int row = 0; row < height; ++row) {
        if (position >= data.size()) { return false; }
        const auto rowSize = static_cast<std::size_t>(
            static_cast<std::uint8_t>(data[position++]));
        if (rowSize > data.size() - position) { return false; }
        const auto end = position + rowSize;
        std::size_t x = 0;
        while (position < end) {
            if (end - position < 2) { return false; }
            const auto skip = static_cast<std::size_t>(
                static_cast<std::uint8_t>(data[position++]));
            const auto count = static_cast<std::size_t>(
                static_cast<std::uint8_t>(data[position++]));
            const auto remainingWidth = static_cast<std::size_t>(width)
                - std::min(x, static_cast<std::size_t>(width));
            if (count > end - position || skip > remainingWidth) {
                return false;
            }
            if (x > std::numeric_limits<std::size_t>::max() - skip - count) {
                return false;
            }
            x += skip + count;
            position += count;
        }
        if (x > static_cast<std::size_t>(width)) { return false; }
    }
    return true;
}

}
