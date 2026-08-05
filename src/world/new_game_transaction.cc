#include "new_game_transaction.hh"

#include "transaction.hh"

#include <algorithm>
#include <cstring>
#include <limits>
#include <utility>

namespace hojy::world::state {
namespace {

bool validCandidate(const SaveData &saveData,
                    std::int16_t initialSubMapId) noexcept {
    if (!saveData.baseInfo.operator->() || !saveData.charInfo[0]
        || initialSubMapId < 0) {
        return false;
    }
    const auto index = static_cast<std::size_t>(initialSubMapId);
    return index < saveData.subMapInfo.size()
        && index < saveData.subMapLayerInfo.size()
        && index < saveData.subMapEventInfo.size()
        && saveData.subMapInfo[initialSubMapId] != nullptr;
}

bool randomValue(NewGameRandom &random, int minimum, int maximum,
                 std::int16_t &output) {
    const auto value = random.next(minimum, maximum);
    if (value < minimum || value > maximum
        || value < std::numeric_limits<std::int16_t>::min()
        || value > std::numeric_limits<std::int16_t>::max()) {
        return false;
    }
    output = static_cast<std::int16_t>(value);
    return true;
}

}

NewGameCandidate::NewGameCandidate(
        SaveData saveData, Bag bag, Strings strings,
        std::int16_t initialSubMapId, std::uint64_t revision) noexcept
    : saveData_(std::move(saveData)), bag_(std::move(bag)),
      strings_(std::move(strings)), initialSubMapId_(initialSubMapId),
      revision_(revision) {}

bool NewGameCandidate::reroll(NewGameRandom &random) noexcept {
    auto *character = saveData_.charInfo[0];
    if (!character) { return false; }
    auto candidate = *character;
    try {
        if (!randomValue(random, 25, 50, candidate.maxHp)) { return false; }
        candidate.hp = candidate.maxHp;
        if (!randomValue(random, 25, 50, candidate.maxMp)) { return false; }
        candidate.mp = candidate.maxMp;
        if (!randomValue(random, 0, 1, candidate.mpType)
            || !randomValue(random, 1, 10, candidate.hpAddOnLevelUp)
            || !randomValue(random, 25, 30, candidate.attack)
            || !randomValue(random, 25, 30, candidate.speed)
            || !randomValue(random, 25, 30, candidate.defence)
            || !randomValue(random, 25, 30, candidate.medic)
            || !randomValue(random, 25, 30, candidate.poison)
            || !randomValue(random, 25, 30, candidate.depoison)
            || !randomValue(random, 25, 30, candidate.fist)
            || !randomValue(random, 25, 30, candidate.sword)
            || !randomValue(random, 25, 30, candidate.blade)
            || !randomValue(random, 25, 30, candidate.special)
            || !randomValue(random, 25, 30, candidate.throwing)
            || !randomValue(random, 1, 100, candidate.potential)) {
            return false;
        }
    } catch (...) {
        return false;
    }
    *character = candidate;
    return true;
}

bool NewGameCandidate::setIdentity(
        std::string_view encodedName,
        std::string_view encodedHomeSuffix) noexcept {
    if (encodedName.size() > 8
        || encodedName.find('\0') != std::string_view::npos
        || !validCandidate(saveData_, initialSubMapId_)) {
        return false;
    }
    try {
        SaveData candidateSave = saveData_;
        auto *character = candidateSave.charInfo[0];
        auto *subMap = candidateSave.subMapInfo[initialSubMapId_];
        if (!character || !subMap) { return false; }
        std::memset(character->name, 0, sizeof(character->name));
        std::memcpy(character->name, encodedName.data(), encodedName.size());
        std::memset(subMap->name, 0, sizeof(subMap->name));
        std::memcpy(subMap->name, encodedName.data(), encodedName.size());
        const auto suffixLength = std::min<std::size_t>(
            encodedHomeSuffix.size(), sizeof(subMap->name) - encodedName.size());
        std::memcpy(subMap->name + encodedName.size(),
                    encodedHomeSuffix.data(), suffixLength);

        Strings candidateStrings;
        if (!strings_.buildForSave(candidateSave, candidateStrings)) {
            return false;
        }
        saveData_.swap(candidateSave);
        strings_.swap(candidateStrings);
        return true;
    } catch (...) {
        return false;
    }
}

std::optional<NewGameCandidate> makeNewGameCandidate(
        SaveData saveData, Bag bag, std::int16_t initialSubMapId) {
    if (!validCandidate(saveData, initialSubMapId)) {
        return std::nullopt;
    }
    Strings strings;
    if (!gStrings.buildForSave(saveData, strings)) {
        return std::nullopt;
    }
    return NewGameCandidate(
        std::move(saveData), std::move(bag), std::move(strings),
        initialSubMapId, stateRevision());
}

std::optional<NewGameCandidate> prepareNewGameCandidate(
        NewGameRandom &random, std::int16_t initialSubMapId) {
    SaveData saveData;
    Bag bag;
    if (!saveData.newGame(bag)) { return std::nullopt; }
    auto candidate = makeNewGameCandidate(
        std::move(saveData), std::move(bag), initialSubMapId);
    if (!candidate || !candidate->reroll(random)) {
        return std::nullopt;
    }
    return candidate;
}

NewGameActivation::NewGameActivation(NewGameCandidate candidate) noexcept
    : candidate_(std::move(candidate)), active_(true) {
    gSaveData.swap(candidate_.saveData_);
    gBag.swap(candidate_.bag_);
    gStrings.swap(candidate_.strings_);
}

NewGameActivation::NewGameActivation(NewGameActivation &&other) noexcept
    : candidate_(std::move(other.candidate_)), active_(other.active_) {
    other.active_ = false;
}

NewGameActivation::~NewGameActivation() {
    rollback();
}

void NewGameActivation::rollback() noexcept {
    if (!active_) { return; }
    gStrings.swap(candidate_.strings_);
    gBag.swap(candidate_.bag_);
    gSaveData.swap(candidate_.saveData_);
    active_ = false;
}

void NewGameActivation::finalize() noexcept {
    if (!active_) { return; }
    active_ = false;
    bumpStateRevision();
}

std::optional<NewGameActivation> activateNewGameCandidate(
        NewGameCandidate &&candidate) {
    if (candidate.revision_ != stateRevision()) {
        return std::nullopt;
    }
    return NewGameActivation(std::move(candidate));
}

static_assert(noexcept(std::declval<SaveData &>().swap(
    std::declval<SaveData &>())));
static_assert(noexcept(std::declval<Bag &>().swap(
    std::declval<Bag &>())));
static_assert(noexcept(std::declval<Strings &>().swap(
    std::declval<Strings &>())));

}
