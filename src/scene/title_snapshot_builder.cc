#include "title_snapshot_builder.hh"

#include "world/character.hh"
#include "world/character_style.hh"
#include "world/strings.hh"

#include <tuple>
#include <utility>

namespace hojy::scene {
namespace {

TitlePreviewPropertySnapshot property(
        int row, int column, const std::wstring &label,
        std::int16_t value, std::int16_t threshold,
        int mpType = -1) {
    TitlePreviewPropertySnapshot result;
    result.row = row;
    result.column = column;
    result.value = value;
    result.displayText = label + L": " + std::to_wstring(value);
    result.highlighted = value >= threshold;
    result.shadow = result.highlighted;
    if (result.highlighted) {
        result.background = {216, 20, 24};
    }
    if (mpType >= 0) {
        std::tie(result.foreground.red,
                 result.foreground.green,
                 result.foreground.blue) =
            ::hojy::world::state::calcColorForMpType(mpType);
    } else if (result.highlighted) {
        result.foreground = {252, 236, 132};
    } else {
        result.foreground = {216, 20, 24};
    }
    return result;
}

}

TitleNameEntrySnapshot buildTitleNameEntrySnapshot(std::wstring name) {
    TitleNameEntrySnapshot result;
    result.name = std::move(name);
    result.displayText = GETTEXT(41) + L'\2' + result.name;
    return result;
}

TitlePreviewSnapshot buildTitlePreviewSnapshot(
        const std::wstring &name,
        const ::hojy::world::state::CharacterData &character,
        bool showPotential,
        int windowBorder,
        int confirmationIndex,
        std::uint64_t generation) {
    TitlePreviewSnapshot result;
    result.prompt = L'\2' + name + L"  \1" + GETTEXT(100);
    result.choices = {GETTEXT(78), GETTEXT(79)};
    result.windowBorder = windowBorder;
    result.confirmationIndex = confirmationIndex;
    result.generation = generation;
    result.properties.reserve(showPotential ? 13 : 12);
    result.properties.push_back(property(
        0, 0, GETTEXT(26), character.maxMp, 50, character.mpType));
    result.properties.push_back(property(
        0, 1, GETTEXT(101), character.attack, 30));
    result.properties.push_back(property(
        0, 2, GETTEXT(9), character.speed, 30));
    result.properties.push_back(property(
        0, 3, GETTEXT(102), character.defence, 30));
    result.properties.push_back(property(
        1, 0, GETTEXT(25), character.maxHp, 50));
    result.properties.push_back(property(
        1, 1, GETTEXT(103), character.medic, 30));
    result.properties.push_back(property(
        1, 2, GETTEXT(104), character.poison, 30));
    result.properties.push_back(property(
        1, 3, GETTEXT(105), character.depoison, 30));
    result.properties.push_back(property(
        2, 0, GETTEXT(106), character.fist, 30));
    result.properties.push_back(property(
        2, 1, GETTEXT(107), character.sword, 30));
    result.properties.push_back(property(
        2, 2, GETTEXT(108), character.blade, 30));
    result.properties.push_back(property(
        2, 3, GETTEXT(109), character.special, 30));
    if (showPotential) {
        result.properties.push_back(property(
            2, 4, GETTEXT(29), character.potential, 100));
    }
    return result;
}

}
