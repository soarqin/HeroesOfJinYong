#include "item_snapshot_builder.hh"

#include "content/constants.hh"
#include "core/config.hh"
#include "world/savedata.hh"
#include "world/strings.hh"

#include <fmt/xchar.h>

#include <cstdlib>
#include <string>

namespace hojy::scene {
namespace {

void appendWrapped(std::vector<std::wstring> &lines, std::wstring fragment) {
    if (fragment.empty()) { return; }
    if (lines.empty() || lines.back().size() >= 24) {
        lines.emplace_back();
    }
    lines.back() += std::move(fragment);
}

std::optional<ItemViewEntrySnapshot> buildEntry(
        std::int16_t itemId, std::int16_t count,
        std::optional<std::pair<int, int>> currentPosition) {
    if (itemId < 0 || count <= 0
        || static_cast<std::size_t>(itemId)
            >= ::hojy::world::state::gSaveData.itemInfo.size()) {
        return std::nullopt;
    }
    const auto *item = ::hojy::world::state::gSaveData.itemInfo[itemId];
    if (!item) { return std::nullopt; }

    ItemViewEntrySnapshot entry;
    entry.itemId = itemId;
    entry.count = count;
    entry.displayText = count > 1
        ? fmt::format(L"{} x{}", GETITEMNAME(itemId), count)
        : GETITEMNAME(itemId);
    if (item->user >= 0
        && static_cast<std::size_t>(item->user)
            < ::hojy::world::state::gSaveData.charInfo.size()
        && ::hojy::world::state::gSaveData.charInfo[item->user]) {
        entry.displayText += L"  (" + GETCHARNAME(item->user) + L')';
    }

    if (itemId == ::hojy::content::ItemIDCompass
        && ::hojy::world::state::gSaveData.baseInfo.operator->()) {
        const auto *base =
            ::hojy::world::state::gSaveData.baseInfo.operator->();
        const auto x = currentPosition ? currentPosition->first : base->mainX;
        const auto y = currentPosition ? currentPosition->second : base->mainY;
        entry.description = core::config.shipLogicEnabled()
            ? fmt::format(GETTEXT(40), x, y, base->shipX, base->shipY)
            : fmt::format(GETTEXT(116), x, y);
    } else {
        entry.description = GETITEMDESC(itemId);
    }

    if (item->itemType == 1 || item->itemType == 2) {
        if (item->charOnly >= 0
            && static_cast<std::size_t>(item->charOnly)
                < ::hojy::world::state::gSaveData.charInfo.size()
            && ::hojy::world::state::gSaveData.charInfo[item->charOnly]) {
            appendWrapped(entry.requirementLines,
                          L" " + GETCHARNAME(item->charOnly));
        }
        if (item->reqMpType == 0 || item->reqMpType == 1) {
            appendWrapped(entry.requirementLines,
                          fmt::format(L" {}={}", GETTEXT(5),
                                      GETTEXT(119 + item->reqMpType)));
        }
#define HOJY_APPEND_REQUIREMENT(field, textId) \
        if (item->field != 0) { \
            appendWrapped(entry.requirementLines, fmt::format( \
                L" {}{}{}", GETTEXT(textId), \
                GETTEXT(121 + (item->field < 0 ? 1 : 0)), \
                std::abs(item->field))); \
        }
        HOJY_APPEND_REQUIREMENT(reqMp, 26)
        HOJY_APPEND_REQUIREMENT(reqAttack, 101)
        HOJY_APPEND_REQUIREMENT(reqSpeed, 9)
        HOJY_APPEND_REQUIREMENT(reqPoison, 104)
        HOJY_APPEND_REQUIREMENT(reqMedic, 103)
        HOJY_APPEND_REQUIREMENT(reqDepoison, 105)
        HOJY_APPEND_REQUIREMENT(reqFist, 106)
        HOJY_APPEND_REQUIREMENT(reqSword, 107)
        HOJY_APPEND_REQUIREMENT(reqBlade, 108)
        HOJY_APPEND_REQUIREMENT(reqSpecial, 123)
        HOJY_APPEND_REQUIREMENT(reqThrowing, 109)
        HOJY_APPEND_REQUIREMENT(reqPotential, 29)
#undef HOJY_APPEND_REQUIREMENT
        if (!entry.requirementLines.empty()) {
            entry.requirementTitle = GETTEXT(116 + item->itemType);
        }
    }

    if (item->itemType >= 1 && item->itemType <= 4) {
        if (item->skillId > 0) {
            appendWrapped(entry.effectLines,
                          fmt::format(L"{}{}", GETTEXT(130),
                                      GETSKILLNAME(item->skillId)));
        }
        if (item->addDoubleAttack) {
            appendWrapped(entry.effectLines,
                          fmt::format(L"{}{}", GETTEXT(130), GETTEXT(22)));
        }
        if (item->changeMpType > 0) {
            appendWrapped(entry.effectLines, fmt::format(
                L"{}{}{}", GETTEXT(5), GETTEXT(128),
                GETTEXT(item->changeMpType == 1 ? 120 : 129)));
        }
#define HOJY_APPEND_EFFECT(field, textId) \
        if (item->field != 0) { \
            appendWrapped(entry.effectLines, fmt::format( \
                L" {}{:+}", GETTEXT(textId), item->field)); \
        }
        HOJY_APPEND_EFFECT(addHp, 25)
        HOJY_APPEND_EFFECT(addMaxHp, 25)
        HOJY_APPEND_EFFECT(addPoisoned, 3)
        HOJY_APPEND_EFFECT(addStamina, 4)
        HOJY_APPEND_EFFECT(addMp, 26)
        HOJY_APPEND_EFFECT(addMaxMp, 26)
        HOJY_APPEND_EFFECT(addAttack, 101)
        HOJY_APPEND_EFFECT(addSpeed, 9)
        HOJY_APPEND_EFFECT(addDefence, 102)
        HOJY_APPEND_EFFECT(addMedic, 103)
        HOJY_APPEND_EFFECT(addPoison, 104)
        HOJY_APPEND_EFFECT(addDepoison, 105)
        HOJY_APPEND_EFFECT(addAntipoison, 14)
        HOJY_APPEND_EFFECT(addFist, 106)
        HOJY_APPEND_EFFECT(addSword, 107)
        HOJY_APPEND_EFFECT(addBlade, 108)
        HOJY_APPEND_EFFECT(addSpecial, 123)
        HOJY_APPEND_EFFECT(addThrowing, 109)
        HOJY_APPEND_EFFECT(addKnowledge, 20)
        HOJY_APPEND_EFFECT(addIntegrity, 21)
        HOJY_APPEND_EFFECT(addPoisonAmp, 23)
#undef HOJY_APPEND_EFFECT
        if (!entry.effectLines.empty()) {
            entry.effectTitle = GETTEXT(123 + item->itemType);
        }
    }
    return entry;
}

std::vector<ItemViewEntrySnapshot> build(
        const std::vector<ItemSelectionEntry> &items,
        std::optional<std::pair<int, int>> currentPosition,
        bool battleOnly) {
    std::vector<ItemViewEntrySnapshot> result;
    result.reserve(items.size());
    for (const auto &[itemId, count]: items) {
        if (battleOnly) {
            if (itemId < 0 || static_cast<std::size_t>(itemId)
                    >= ::hojy::world::state::gSaveData.itemInfo.size()) {
                continue;
            }
            const auto *item =
                ::hojy::world::state::gSaveData.itemInfo[itemId];
            if (!item || (item->itemType != 3 && item->itemType != 4)) {
                continue;
            }
        }
        auto entry = buildEntry(itemId, count, currentPosition);
        if (entry) { result.push_back(std::move(*entry)); }
    }
    return result;
}

}

std::vector<ItemViewEntrySnapshot> buildItemViewSnapshot(
        const std::vector<ItemSelectionEntry> &items,
        std::optional<std::pair<int, int>> currentPosition) {
    return build(items, currentPosition, false);
}

std::vector<ItemViewEntrySnapshot> buildBattleItemViewSnapshot(
        const std::vector<ItemSelectionEntry> &items) {
    return build(items, std::nullopt, true);
}

}
