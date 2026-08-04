#include "scene/colorpalette.hh"
#include "test_support.hh"

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

void writePalette(const std::filesystem::path &path, std::size_t bytes) {
    std::ofstream output(path, std::ios::binary);
    for (std::size_t i = 0; i < bytes; ++i) {
        const auto value = static_cast<char>(i % 64);
        output.write(&value, 1);
    }
}

void paletteLoadsTransactionally() {
    const auto oldPath = std::filesystem::current_path();
    const auto directory = std::filesystem::temp_directory_path() / "hojy-palette-tests";
    std::error_code error;
    std::filesystem::remove_all(directory, error);
    std::filesystem::create_directories(directory);
    std::filesystem::current_path(directory);

    hojy::scene::ColorPalette palette;
    std::array<std::uint32_t, 256> initial{};
    initial.fill(0x12345678U);
    palette.create(initial);

    writePalette("GOOD.COL", 256 * 3);
    HOJY_CHECK_EQ(palette.load("GOOD"), true);
    HOJY_CHECK_EQ(palette.colors()[0], 0U);
    HOJY_CHECK_EQ(palette.colors()[1], 0xFF0C1014U);

    writePalette("SHORT.COL", 255 * 3);
    const auto before = palette.colors()[1];
    HOJY_CHECK_EQ(palette.load("SHORT"), false);
    HOJY_CHECK_EQ(palette.colors()[1], before);

    std::filesystem::current_path(oldPath, error);
    std::filesystem::remove_all(directory, error);
}

}

int main() {
    try {
        paletteLoadsTransactionally();
    } catch (const std::exception &exception) {
        std::cerr << exception.what() << '\n';
        return 1;
    }
    return 0;
}
