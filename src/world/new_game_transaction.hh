#pragma once

#include "bag.hh"
#include "savedata.hh"
#include "strings.hh"

#include <cstdint>
#include <optional>
#include <string_view>

namespace hojy::world::state {

class NewGameActivation;

class NewGameRandom {
public:
    virtual ~NewGameRandom() = default;
    virtual int next(int minimum, int maximum) = 0;
};

class NewGameCandidate final {
public:
    NewGameCandidate(NewGameCandidate &&) noexcept = default;
    NewGameCandidate &operator=(NewGameCandidate &&) noexcept = default;
    NewGameCandidate(const NewGameCandidate &) = delete;
    NewGameCandidate &operator=(const NewGameCandidate &) = delete;

    [[nodiscard]] bool reroll(NewGameRandom &random) noexcept;
    [[nodiscard]] bool setIdentity(
        std::string_view encodedName,
        std::string_view encodedHomeSuffix) noexcept;

    [[nodiscard]] const SaveData &saveData() const noexcept {
        return saveData_;
    }
    [[nodiscard]] const Bag &bag() const noexcept { return bag_; }
    [[nodiscard]] const Strings &strings() const noexcept { return strings_; }
    [[nodiscard]] std::int16_t initialSubMapId() const noexcept {
        return initialSubMapId_;
    }

private:
    friend std::optional<NewGameCandidate> makeNewGameCandidate(
        SaveData, Bag, std::int16_t);
    friend std::optional<NewGameCandidate> prepareNewGameCandidate(
        NewGameRandom &, std::int16_t);
    friend class NewGameActivation;
    friend std::optional<NewGameActivation> activateNewGameCandidate(
        NewGameCandidate &&);

    NewGameCandidate(SaveData saveData, Bag bag, Strings strings,
                     std::int16_t initialSubMapId,
                     std::uint64_t revision) noexcept;

    SaveData saveData_;
    Bag bag_;
    Strings strings_;
    std::int16_t initialSubMapId_ = -1;
    std::uint64_t revision_ = 0;
};

class NewGameActivation final {
public:
    NewGameActivation(NewGameActivation &&other) noexcept;
    NewGameActivation &operator=(NewGameActivation &&) = delete;
    NewGameActivation(const NewGameActivation &) = delete;
    NewGameActivation &operator=(const NewGameActivation &) = delete;
    ~NewGameActivation();

    // Explicitly cancel the activation before restoring dependent scene state.
    // The destructor remains a safety net for callers that abandon the scope.
    void rollback() noexcept;
    void finalize() noexcept;

    [[nodiscard]] std::int16_t initialSubMapId() const noexcept {
        return candidate_.initialSubMapId_;
    }

private:
    friend std::optional<NewGameActivation> activateNewGameCandidate(
        NewGameCandidate &&);

    explicit NewGameActivation(NewGameCandidate candidate) noexcept;

    NewGameCandidate candidate_;
    bool active_ = false;
};

[[nodiscard]] std::optional<NewGameCandidate> makeNewGameCandidate(
    SaveData saveData, Bag bag, std::int16_t initialSubMapId);

[[nodiscard]] std::optional<NewGameCandidate> prepareNewGameCandidate(
    NewGameRandom &random, std::int16_t initialSubMapId);

[[nodiscard]] std::optional<NewGameActivation> activateNewGameCandidate(
    NewGameCandidate &&candidate);

}
