#include "content/factors.hh"
#include "test_support.hh"
#include "world/new_game_transaction.hh"
#include "world/strings.hh"
#include "world/transaction.hh"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using hojy::world::state::Bag;
using hojy::world::state::CharacterData;
using hojy::world::state::NewGameCandidate;
using hojy::world::state::NewGameRandom;
using hojy::world::state::SaveData;
using hojy::world::state::SubMapData;

template<typename Serializable, typename Value>
void loadValues(Serializable &serializable, const std::vector<Value> &values) {
    const std::string bytes(
        reinterpret_cast<const char *>(values.data()),
        values.size() * sizeof(Value));
    HOJY_CHECK_EQ(serializable.deserialize(bytes), true);
}

SaveData makeSave(const char *name, std::int16_t maxHp) {
    SaveData save;
    auto &base = *save.baseInfo.operator->();
    for (auto &member: base.members) { member = -1; }
    base.members[0] = 0;
    for (auto &item: base.items) { item = {-1, 0}; }
    base.items[0] = {1, 3};

    CharacterData character{};
    character.id = 0;
    character.maxHp = maxHp;
    character.hp = maxHp;
    std::strncpy(character.name, name, sizeof(character.name));
    loadValues(save.charInfo, std::vector<CharacterData>{character});

    SubMapData subMap{};
    subMap.id = 0;
    std::strncpy(subMap.name, "HOME", sizeof(subMap.name));
    loadValues(save.subMapInfo, std::vector<SubMapData>{subMap});
    save.subMapLayerInfo.resize(1);
    save.subMapEventInfo.resize(1);
    return save;
}

Bag makeBag(const SaveData &save) {
    Bag bag;
    HOJY_CHECK_EQ(bag.syncFrom(*save.baseInfo.operator->()), true);
    return bag;
}

class SequenceRandom final : public NewGameRandom {
public:
    explicit SequenceRandom(std::vector<int> values, int throwAt = -1)
        : values_(std::move(values)), throwAt_(throwAt) {}

    int next(int minimum, int maximum) override {
        if (throwAt_ >= 0 && index_ == throwAt_) {
            throw std::runtime_error("random failure");
        }
        if (index_ >= values_.size()) {
            throw std::runtime_error("random sequence exhausted");
        }
        const auto value = values_[index_++];
        if (value < minimum || value > maximum) {
            throw std::runtime_error("random value outside requested range");
        }
        return value;
    }

private:
    std::vector<int> values_;
    int throwAt_ = -1;
    std::size_t index_ = 0;
};

std::vector<int> randomValues(int offset = 0) {
    return {
        25 + offset, 26 + offset, offset % 2, 1 + offset,
        25 + offset, 26 + offset, 27 + offset, 28 + offset,
        29 + offset, 25 + offset, 26 + offset, 27 + offset,
        28 + offset, 29 + offset, 25 + offset, 1 + offset,
    };
}

class GlobalStateGuard final {
public:
    GlobalStateGuard()
        : save_(hojy::world::state::gSaveData),
          bag_(hojy::world::state::gBag),
          strings_(hojy::world::state::gStrings),
          initialSubMapId_(hojy::content::gFactors.initSubMapId) {}

    ~GlobalStateGuard() {
        hojy::world::state::gSaveData = std::move(save_);
        hojy::world::state::gBag = std::move(bag_);
        hojy::world::state::gStrings = std::move(strings_);
        hojy::content::gFactors.initSubMapId = initialSubMapId_;
    }

private:
    SaveData save_;
    Bag bag_;
    hojy::world::state::Strings strings_;
    std::int16_t initialSubMapId_ = -1;
};

NewGameCandidate makeCandidate(const char *name, int offset = 0) {
    auto save = makeSave(name, 10);
    auto bag = makeBag(save);
    auto candidate = hojy::world::state::makeNewGameCandidate(
        std::move(save), std::move(bag), 0);
    HOJY_CHECK_EQ(candidate.has_value(), true);
    SequenceRandom random(randomValues(offset));
    HOJY_CHECK_EQ(candidate->reroll(random), true);
    return std::move(*candidate);
}

void testCandidatePreparationAndRerollDoNotTouchLiveWorld() {
    GlobalStateGuard guard;
    hojy::content::gFactors.initSubMapId = 0;
    hojy::world::state::gSaveData = makeSave("LIVE", 77);
    hojy::world::state::gBag = makeBag(hojy::world::state::gSaveData);
    hojy::world::state::gStrings.saveDataLoaded();

    auto candidate = makeCandidate("DRAFT");
    HOJY_CHECK_EQ(hojy::world::state::gSaveData.charInfo[0]->maxHp, 77);
    HOJY_CHECK_EQ(hojy::world::state::gBag[1], 3);
    HOJY_CHECK_EQ(candidate.saveData().charInfo[0]->maxHp, 25);
    HOJY_CHECK_EQ(candidate.saveData().charInfo[0]->maxMp, 26);
    HOJY_CHECK_EQ(candidate.saveData().charInfo[0]->potential, 1);
}

void testRandomFailureLeavesCandidateUnchanged() {
    GlobalStateGuard guard;
    auto save = makeSave("DRAFT", 44);
    auto bag = makeBag(save);
    auto candidate = hojy::world::state::makeNewGameCandidate(
        std::move(save), std::move(bag), 0);
    HOJY_CHECK_EQ(candidate.has_value(), true);
    const auto before = *candidate->saveData().charInfo[0];
    SequenceRandom random(randomValues(), 4);
    HOJY_CHECK_EQ(candidate->reroll(random), false);
    HOJY_CHECK_EQ(
        std::memcmp(&before, candidate->saveData().charInfo[0], sizeof(before)),
        0);
}

void testIdentityUpdateIsTransactionalAndRebuildsStrings() {
    GlobalStateGuard guard;
    auto candidate = makeCandidate("DRAFT");
    HOJY_CHECK_EQ(candidate.setIdentity("HERO", " HOME"), true);
    HOJY_CHECK_EQ(
        std::string(candidate.saveData().charInfo[0]->name, 4), "HERO");
    HOJY_CHECK_EQ(
        std::string(candidate.saveData().subMapInfo[0]->name, 9),
        "HERO HOME");
    HOJY_CHECK_EQ(candidate.strings()(
        hojy::world::state::Strings::CharName, 0), L"HERO");

    const auto before = *candidate.saveData().charInfo[0];
    HOJY_CHECK_EQ(candidate.setIdentity("TOO-LONG-NAME", " HOME"), false);
    HOJY_CHECK_EQ(
        std::memcmp(&before, candidate.saveData().charInfo[0], sizeof(before)),
        0);
}

void testActivationRollsBackUnlessFinalizedAndRejectsStaleCandidate() {
    GlobalStateGuard guard;
    hojy::world::state::gSaveData = makeSave("LIVE", 77);
    hojy::world::state::gBag = makeBag(hojy::world::state::gSaveData);
    hojy::world::state::gStrings.saveDataLoaded();
    const auto revision = hojy::world::state::stateRevision();

    {
        auto candidate = makeCandidate("ROLLBACK");
        HOJY_CHECK_EQ(candidate.setIdentity("ROLL", " HOME"), true);
        auto activation = hojy::world::state::activateNewGameCandidate(
            std::move(candidate));
        HOJY_CHECK_EQ(activation.has_value(), true);
        HOJY_CHECK_EQ(hojy::world::state::gSaveData.charInfo[0]->maxHp, 25);
        HOJY_CHECK_EQ(GETCHARNAME(0), L"ROLL");
    }
    HOJY_CHECK_EQ(hojy::world::state::gSaveData.charInfo[0]->maxHp, 77);
    HOJY_CHECK_EQ(GETCHARNAME(0), L"LIVE");
    HOJY_CHECK_EQ(hojy::world::state::stateRevision(), revision);

    auto committed = makeCandidate("COMMIT", 0);
    HOJY_CHECK_EQ(committed.setIdentity("DONE", " HOME"), true);
    auto activation = hojy::world::state::activateNewGameCandidate(
        std::move(committed));
    HOJY_CHECK_EQ(activation.has_value(), true);
    activation->finalize();
    HOJY_CHECK_EQ(GETCHARNAME(0), L"DONE");
    HOJY_CHECK_EQ(hojy::world::state::stateRevision(), revision + 1);

    auto stale = makeCandidate("STALE");
    hojy::world::state::bumpStateRevision();
    const auto before = *hojy::world::state::gSaveData.charInfo[0];
    HOJY_CHECK_EQ(
        hojy::world::state::activateNewGameCandidate(std::move(stale)).has_value(),
        false);
    HOJY_CHECK_EQ(
        std::memcmp(&before, hojy::world::state::gSaveData.charInfo[0],
                    sizeof(before)),
        0);
}

void testExplicitRollbackRestoresAllActivatedWorldState() {
    GlobalStateGuard guard;
    hojy::world::state::gSaveData = makeSave("LIVE", 77);
    hojy::world::state::gBag = makeBag(hojy::world::state::gSaveData);
    hojy::world::state::gStrings.saveDataLoaded();
    const auto revision = hojy::world::state::stateRevision();

    auto candidate = makeCandidate("DRAFT", 0);
    HOJY_CHECK_EQ(candidate.setIdentity("NEW", " HOME"), true);
    auto activation = hojy::world::state::activateNewGameCandidate(
        std::move(candidate));
    HOJY_CHECK_EQ(activation.has_value(), true);
    HOJY_CHECK_EQ(hojy::world::state::gSaveData.charInfo[0]->maxHp, 25);
    HOJY_CHECK_EQ(GETCHARNAME(0), L"NEW");

    activation->rollback();

    HOJY_CHECK_EQ(hojy::world::state::gSaveData.charInfo[0]->maxHp, 77);
    HOJY_CHECK_EQ(GETCHARNAME(0), L"LIVE");
    HOJY_CHECK_EQ(hojy::world::state::gBag[1], 3);
    HOJY_CHECK_EQ(hojy::world::state::stateRevision(), revision);
}

}

int main() {
    try {
        testCandidatePreparationAndRerollDoNotTouchLiveWorld();
        testRandomFailureLeavesCandidateUnchanged();
        testIdentityUpdateIsTransactionalAndRebuildsStrings();
        testActivationRollsBackUnlessFinalizedAndRejectsStaleCandidate();
        testExplicitRollbackRestoresAllActivatedWorldState();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
