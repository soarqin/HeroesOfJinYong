#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace hojy::scene {

struct TitleColorSnapshot final {
    std::uint8_t red = 0;
    std::uint8_t green = 0;
    std::uint8_t blue = 0;
};

struct TitleMainMenuSnapshot final {
    int selectedIndex = 0;
};

struct TitleLoadMenuSnapshot final {
    int selectedIndex = 0;
};

struct TitleNameEntrySnapshot final {
    std::wstring name;
    std::wstring displayText;
};

struct TitlePreviewPropertySnapshot final {
    int row = 0;
    int column = 0;
    std::int16_t value = 0;
    std::wstring displayText;
    TitleColorSnapshot foreground;
    TitleColorSnapshot background;
    bool highlighted = false;
    bool shadow = false;
};

struct TitlePreviewSnapshot final {
    std::wstring prompt;
    std::vector<TitlePreviewPropertySnapshot> properties;
    std::array<std::wstring, 2> choices;
    int confirmationIndex = -1;
    std::uint64_t generation = 0;
};

using TitleScreenSnapshot = std::variant<
    TitleMainMenuSnapshot,
    TitleLoadMenuSnapshot,
    TitleNameEntrySnapshot,
    TitlePreviewSnapshot>;

}
