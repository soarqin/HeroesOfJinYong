#pragma once

#include <array>
#include <string>
#include <cstdint>

namespace hojy::data {

class ColorPalette final {
public:
    void load(const std::string &name);
    constexpr size_t size() { return palette_.size(); }
    [[nodiscard]] const std::uint32_t *colors() const { return palette_.data(); }

private:
    std::array<std::uint32_t, 256> palette_;
};

extern ColorPalette normalPalette, endPalette;

}
