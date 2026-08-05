#include "warfield.hh"
#include "warfield_load.hh"

#include "logic/rle.hh"
#include "content/grpdata.hh"
#include "content/warfielddata.hh"
#include "world/action.hh"
#include "world/savedata.hh"
#include "window_command.hh"

#include <fmt/xchar.h>
#include <memory>
#include <new>
#include <cstdint>
#include <limits>
#include <set>
#include <utility>
#include <vector>

namespace hojy::scene {
Warfield::Warfield(Renderer *renderer, int x, int y, int width, int height, std::pair<int, int> scale):
    Map(renderer, x, y, width, height, scale),
    battleRandom_(battleGameRandom_),
    drawingTerrainTex2_(Texture::create(renderer, auxWidth_, auxHeight_)),
    presentationOwnerState_(std::make_shared<PresentationOwnerState>()) {
    presentationOwnerState_->owner = this;
    resourcesReady_ = drawingTerrainTex_ != nullptr && drawingTerrainTex2_ != nullptr;
    if (drawingTerrainTex2_) {
        resourcesReady_ = drawingTerrainTex2_->enableBlendMode(true) && resourcesReady_;
    }
    setStage(Idle);
    if (!resourcesReady_) { return; }
    try {
        std::vector<std::vector<std::string>> candidateFightTextures(FightTextureListCount);
        for (size_t i = 0; i < candidateFightTextures.size(); ++i) {
            if (!::hojy::content::GrpData::loadData(
                    fmt::format("FIGHT{:03}.IDX", i), fmt::format("FIGHT{:03}.GRP", i),
                    candidateFightTextures[i])) {
                resourcesReady_ = false;
                return;
            }
            for (const auto &texture: candidateFightTextures[i]) {
                if (!texture.empty() && !logic::validateRleData(texture)) {
                    resourcesReady_ = false;
                    return;
                }
            }
        }
        fightTexData_ = std::move(candidateFightTextures);
    } catch (const std::bad_alloc &) {
        resourcesReady_ = false;
    }
}

Warfield::~Warfield() {
    invalidatePresentationSession();
    if (presentationOwnerState_) {
        presentationOwnerState_->owner = nullptr;
        presentationOwnerState_.reset();
    }
    delete drawingTerrainTex2_;
    delete statusPanel_;
}

bool Warfield::beginPresentationSession() {
    try {
        return presentationSession_.begin() != 0;
    } catch (const std::bad_alloc &) {
        presentationSession_.invalidate();
        return false;
    }
}

void Warfield::invalidatePresentationSession() noexcept {
    presentationSession_.invalidate();
}

std::uint64_t Warfield::presentationSessionToken() const noexcept {
    return presentationSession_.token();
}

BattlePresentationSession::Handle Warfield::presentationSessionHandle() const noexcept {
    return presentationSession_.handle();
}

std::weak_ptr<Warfield::PresentationOwnerState>
Warfield::presentationOwnerHandle() const noexcept {
    return presentationOwnerState_;
}

bool Warfield::isCurrentPresentationSession(std::uint64_t token) const noexcept {
    return presentationSession_.matches(token);
}

bool Warfield::matchesPresentationContext(
        std::uint64_t sessionToken, std::uint64_t actionGeneration,
        BattlePresentationStage expectedStage,
        std::int16_t expectedActorId) const noexcept {
    if (!isCurrentPresentationSession(sessionToken)
        || !isValidBattlePresentationRequest(
            sessionToken, actionGeneration, expectedStage)
        || actionGeneration != presentationGeneration_
        || expectedStage != presentationStage_) {
        return false;
    }
    switch (expectedStage) {
    case BattlePresentationStage::Any:
        break;
    case BattlePresentationStage::PlayerMenu:
        if (stage_ != PlayerMenu) { return false; }
        break;
    case BattlePresentationStage::DirectionSelection:
    case BattlePresentationStage::SkillLevelUp:
    case BattlePresentationStage::ItemResult:
    case BattlePresentationStage::ItemSelection:
    case BattlePresentationStage::StatusSelection:
        if (stage_ != PoppingUp) { return false; }
        break;
    case BattlePresentationStage::FinishMessages:
        if (stage_ != Finished) { return false; }
        break;
    }
    if (expectedActorId >= 0
        && (!currentActor_ || currentActor_->id != expectedActorId)) {
        return false;
    }
    return true;
}

void Warfield::clearActionState(bool clearPopupNumbers) {
    actIndex_ = -1;
    actId_ = -1;
    actLevel_ = 0;
    actItemSlot_ = -1;
    skillLevelup_ = false;
    pendingSkillLevelUpActorId_ = -1;
    pendingSkillLevelUpSkillId_ = -1;
    pendingSkillLevelUpSkillIndex_ = -1;
    pendingItemResultActorId_ = -1;
    pendingItemResultItemId_ = -1;
    effectId_ = -1;
    effectTexIdx_ = -1;
    effectOverlaySnapshot_ = {};
    fightTexIdx_ = -1;
    fightTexCount_ = 0;
    fightFrame_ = 0;
    attackTimesLeft_ = 0;
    fightTex_ = nullptr;
    actionTargets_.clear();
    if (clearPopupNumbers) {
        popupNumbers_.clear();
    }
}

void Warfield::cleanup() {
    invalidatePresentationSession();
    discardBattleSession();
    battleRandom_.clear();
    battleBag_ = {};
    battleBagActive_ = false;
    pendingBattleMusic_ = -1;
    presentationCleanupRequested_ = true;
    pendingFinishMessages_.reset();
    pendingAutoAction_ = nullptr;
    pendingSkillLevelUpContinuation_ = nullptr;
    pendingInputAction_ = nullptr;
    pendingModeKey_ = InputKey::None;
    hasPendingModeKey_ = false;
    resumeAutoAttack_ = false;
    currentActor_ = nullptr;
    for (auto &cell: cellInfo_) {
        cell.charInfo = nullptr;
        cell.insideMovingArea = 0;
    }
    turnOrder_.clear();
    charQueue_.clear();
    chars_.clear();
    round_ = 0;
    setStage(Idle);
    knowledge_[0] = knowledge_[1] = 0;
    cursorX_ = 0;
    cursorY_ = 0;
    autoControl_ = false;
    won_ = false;
    selCells_.clear();
    movingPath_.clear();
    markWorldChanged();
    clearActionState(true);
}

void Warfield::abortPresentationState() noexcept {
    invalidatePresentationSession();
    discardBattleSession();
    battleBag_ = {};
    battleBagActive_ = false;
    pendingBattleMusic_ = -1;
    currentActor_ = nullptr;
    pendingAutoAction_ = nullptr;
    pendingSkillLevelUpContinuation_ = nullptr;
    pendingSkillLevelUpActorId_ = -1;
    pendingSkillLevelUpSkillId_ = -1;
    pendingSkillLevelUpSkillIndex_ = -1;
    pendingInputAction_ = nullptr;
    pendingModeKey_ = InputKey::None;
    hasPendingModeKey_ = false;
    turnOrder_.clear();
    charQueue_.clear();
    movingPath_.clear();
    selCells_.clear();
    popupNumbers_.clear();
    presentationCleanupRequested_ = true;
    pendingFinishMessages_.reset();
    resumeAutoAttack_ = false;
    clearActionState(false);
    setStage(Finished);
}

void Warfield::syncBattleParticipantsToWorking() noexcept {
    if (battleParticipants_.size() != chars_.size()) { return; }
    for (std::size_t i = 0; i < chars_.size(); ++i) {
        if (battleParticipants_[i]) {
            battleParticipants_[i]->state() = chars_[i].info;
        }
    }
}

void Warfield::syncBattleParticipantsFromWorking() noexcept {
    if (battleParticipants_.size() != chars_.size()) { return; }
    for (std::size_t i = 0; i < chars_.size(); ++i) {
        if (battleParticipants_[i]) {
            chars_[i].info = battleParticipants_[i]->state();
        }
    }
}

std::optional<battle::ParticipantId> Warfield::participantIndex(
        const CharInfo *character) const noexcept {
    if (!character || chars_.empty()) { return std::nullopt; }
    const auto address = reinterpret_cast<std::uintptr_t>(character);
    const auto begin = reinterpret_cast<std::uintptr_t>(chars_.data());
    const auto span = sizeof(CharInfo) * chars_.size();
    if (address < begin || address - begin >= span
        || (address - begin) % sizeof(CharInfo) != 0) {
        return std::nullopt;
    }
    return static_cast<battle::ParticipantId>(
        (address - begin) / sizeof(CharInfo));
}

battle::InventorySnapshot Warfield::battleInventorySnapshot() const {
    battle::InventorySnapshot snapshot;
    snapshot.reserve(battleBag_.orderedItems().size());
    for (const auto &[itemId, count]: battleBag_.orderedItems()) {
        if (itemId >= 0 && count > 0) {
            snapshot.emplace_back(itemId, count);
        }
    }
    return snapshot;
}

bool Warfield::recordBattleAction(const battle::BattleAction &action) {
    if (battleParticipants_.size() != chars_.size()
        || battleEngine_.status() != battle::EngineStatus::Active) {
        queueBattleAbortTransition();
        abortPresentationState();
        return false;
    }
    try {
        syncBattleParticipantsToWorking();
        const auto recorded = battleEngine_.record(
            action, battleInventorySnapshot());
        syncBattleParticipantsFromWorking();
        if (!recorded) {
            queueBattleAbortTransition();
            abortPresentationState();
        }
        return recorded;
    } catch (const std::bad_alloc &) {
        queueBattleAbortTransition();
        abortPresentationState();
        return false;
    }
}

void Warfield::queueBattleAbortTransition() noexcept {
    const auto sessionToken = presentationSessionToken();
    if (sessionToken == 0) { return; }
    try {
        postCommand([sessionToken](SceneCommandContext &context) {
            context.abortBattle(BattleAbortRequest{sessionToken, false});
        });
    } catch (const std::bad_alloc &) {
        // The engine failure must still roll back even if command allocation
        // is unavailable; Window will reject any stale presentation callbacks.
    }
}

void Warfield::discardBattleSession() noexcept {
    if (!battleParticipants_.empty()) {
        syncBattleParticipantsToWorking();
    }
    if (battleEngine_.status() != battle::EngineStatus::Idle) {
        battleEngine_.abort();
    } else {
        for (auto &participant: battleParticipants_) {
            if (participant) { participant->discard(); }
        }
    }
    if (!battleParticipants_.empty()) {
        syncBattleParticipantsFromWorking();
    }
    battleParticipants_.clear();
}

void Warfield::commitBattleBag() noexcept {
    if (!battleBagActive_) { return; }
    ::hojy::world::state::gBag.swap(battleBag_);
    battleBag_ = {};
    battleBagActive_ = false;
}

bool Warfield::load(std::int16_t warId) {
    if (!resourcesReady_) { return false; }
    const auto *info = ::hojy::content::gWarfieldData.info(warId);
    if (!info) { return false; }
    const auto warMapId = info->warFieldId;
    const auto *warfieldLayers = ::hojy::content::gWarfieldData.layers(warMapId);
    if (!warfieldLayers) { return false; }
    const auto &layers = warfieldLayers->layers;
    const bool mapCached = warMapLoaded_.find(warMapId) != warMapLoaded_.end();
    detail::WarfieldTextureLoad loadedTextures;
    if (mapCached) {
        if (!detail::readWarfieldTextureHeader(texData_, loadedTextures)) {
            return false;
        }
    } else {
        if (!detail::loadWarfieldTextures(
                fmt::format("WDX{:03}", warMapId),
                fmt::format("WMP{:03}", warMapId),
                [](const std::string &idx, const std::string &grp,
                   ::hojy::content::GrpData::DataSet &textures) {
                    return ::hojy::content::GrpData::loadData(idx, grp, textures);
                },
                loadedTextures)) {
            return false;
        }
    }

    const int mapWidth = ::hojy::content::WarFieldWidth;
    const int mapHeight = ::hojy::content::WarFieldHeight;
    const int cellDiffX = loadedTextures.cellWidth / 2;
    const int cellDiffY = loadedTextures.cellHeight / 2;
    const auto size = mapWidth * mapHeight;
    const auto &textureData = mapCached ? texData_ : loadedTextures.textures;
    if (textureData.empty()
        || textureData.size() > static_cast<std::size_t>(std::numeric_limits<std::int16_t>::max())) {
        return false;
    }
    for (const auto &texture: textureData) {
        if (!texture.empty() && !logic::validateRleData(texture)) {
            return false;
        }
    }
    if (!detail::validateWarfieldTextureIds(
            layers[0], layers[1], static_cast<std::size_t>(size),
            textureData.size())) {
        return false;
    }
    for (std::size_t i = 0; i < static_cast<std::size_t>(size); ++i) {
        const auto earthId = static_cast<std::size_t>(layers[0][i] >> 1);
        if (textureData[earthId].empty()) { return false; }
        const auto buildingId = layers[1][i] >> 1;
        if (buildingId > 0
            && textureData[static_cast<std::size_t>(buildingId)].empty()) {
            return false;
        }
    }
    std::vector<CellInfo> cellInfo(static_cast<size_t>(size));

    int x = (mapHeight - 1) * cellDiffX + loadedTextures.offsetX;
    int y = loadedTextures.offsetY;
    int pos = 0;
    for (int j = mapHeight; j; --j) {
        int tx = x, ty = y;
        for (int i = mapWidth; i; --i, ++pos, tx += cellDiffX, ty += cellDiffY) {
            auto &ci = cellInfo[static_cast<size_t>(pos)];
            auto texId = layers[0][pos] >> 1;
            ci.earthId = texId;
            ci.buildingId = layers[1][pos] >> 1;
            ci.blocked = ci.buildingId > 0 || texId >= 179 && texId <= 181 || texId == 261 || texId == 511 || texId >= 662 && texId <= 665 || texId == 674;
        }
        x -= cellDiffX; y += cellDiffY;
    }

    auto nextWarMapLoaded = warMapLoaded_;
    if (!mapCached) {
        detail::commitWarfieldTextureCache(
            nextWarMapLoaded, warMapId, loadedTextures.shared);
    }
    cleanup();
    warId_ = warId;
    mapWidth_ = mapWidth;
    mapHeight_ = mapHeight;
    cellWidth_ = loadedTextures.cellWidth;
    cellHeight_ = loadedTextures.cellHeight;
    offsetX_ = loadedTextures.offsetX;
    offsetY_ = loadedTextures.offsetY;
    if (!mapCached) {
        presentationTextureResetRequested_ = true;
        texData_ = std::move(loadedTextures.textures);
        warMapLoaded_ = std::move(nextWarMapLoaded);
    }
    cellInfo_ = std::move(cellInfo);

    subMapId_ = warMapId;
    resetFrame();
    return true;
}

bool Warfield::getDefaultChars(std::set<std::int16_t> &chars) const {
    if (!resourcesReady_) { return false; }
    const auto *info = ::hojy::content::gWarfieldData.info(warId_);
    if (!info) { return false; }
    if (info->forceMembers[0] >= 0) { return false; }
    for (auto &id: info->defaultMembers) {
        if (id >= 0) { chars.insert(id); }
    }
    return true;
}

bool Warfield::putChars(const std::vector<std::int16_t> &chars) {
    if (!resourcesReady_) { return false; }
    const auto *info = ::hojy::content::gWarfieldData.info(warId_);
    if (!info || cellInfo_.empty() || !chars_.empty() || !battleParticipants_.empty()) {
        return false;
    }
    std::vector<CharInfo> nextChars;
    std::vector<bool> occupied(cellInfo_.size(), false);
    std::vector<std::int16_t> rosterIds;
    rosterIds.reserve(
        ::hojy::content::TeamMemberCount
        + ::hojy::content::WarFieldEnemyCount + chars.size());
    const auto collectRosterId = [&rosterIds](std::int16_t id) {
        if (id >= 0) { rosterIds.push_back(id); }
    };
    if (info->forceMembers[0] >= 0) {
        for (const auto id: info->forceMembers) { collectRosterId(id); }
    } else {
        for (const auto id: chars) { collectRosterId(id); }
    }
    for (const auto id: info->enemy) { collectRosterId(id); }
    if (!detail::validateUniqueWarfieldCharacterIds(rosterIds)) {
        return false;
    }
    auto appendChar = [&](std::uint8_t side, std::int16_t id, std::int16_t x,
                          std::int16_t y, Direction direction) {
        if (id < 0) { return; }
        auto *charInfo = ::hojy::world::state::gSaveData.charInfo[id];
        if (!charInfo) { return; }
        if (x < 0 || x >= mapWidth_ || y < 0 || y >= mapHeight_) { return; }
        const auto index = static_cast<std::size_t>(y) * static_cast<std::size_t>(mapWidth_)
            + static_cast<std::size_t>(x);
        if (index >= occupied.size() || occupied[index]) { return; }
        occupied[index] = true;
        nextChars.emplace_back(CharInfo {side, id, charInfo->headId, x, y, direction, *charInfo});
    };

    if (info->forceMembers[0] >= 0) {
        for (size_t i = 0; i < ::hojy::content::TeamMemberCount; ++i) {
            auto id = info->forceMembers[i];
            appendChar(0, id, info->memberX[i], info->memberY[i], DirLeft);
        }
    } else {
        std::map<std::int16_t, size_t> charMap;
        std::set<size_t> indices;
        for (size_t i = 0; i < ::hojy::content::TeamMemberCount; ++i) {
            auto id = info->defaultMembers[i];
            if (id >= 0) { charMap[id] = i; }
            else { indices.insert(i); }
        }
        for (auto id: chars) {
            auto ite = charMap.find(id);
            size_t index;
            if (ite != charMap.end()) {
                index = ite->second;
            } else {
                if (indices.empty()) { continue; }
                index = *indices.begin();
                indices.erase(indices.begin());
            }
            appendChar(0, id, info->memberX[index], info->memberY[index], DirLeft);
        }
    }
    for (size_t i = 0; i < ::hojy::content::WarFieldEnemyCount; ++i) {
        auto id = info->enemy[i];
        appendChar(1, id, info->enemyX[i], info->enemyY[i], DirRight);
    }

    try {
        auto nextBattleBag = ::hojy::world::state::gBag;
        battle::InventorySnapshot nextInventory;
        nextInventory.reserve(nextBattleBag.orderedItems().size());
        for (const auto &[itemId, count]: nextBattleBag.orderedItems()) {
            if (itemId >= 0 && count > 0) {
                nextInventory.emplace_back(itemId, count);
            }
        }
        battleRandom_.clear();
        for (auto &ci: nextChars) {
            ci.aiEntryStats = battle::snapshotAiStats(ci.info);
            ci.attack = ci.aiEntryStats.attack;
            ci.defence = ci.info.defence;
            ci.persistentEntryMaxMp = ci.info.maxMp;
            ::hojy::world::state::addUpPropFromEquipToChar(&ci.info);
            ci.aiEquipmentBonusStats = battle::captureAiEquipmentBonuses(
                ci.aiEntryStats, ci.info);
            ci.battleEntryMaxMp = ci.info.maxMp;
            if (ci.side == 1) {
                ci.info.hp = ci.info.maxHp;
                ci.info.mp = ci.info.maxMp;
                ci.info.stamina = ::hojy::content::StaminaMax;
            }
        }
        chars_ = std::move(nextChars);
        battleParticipants_.reserve(chars_.size());
        std::vector<battle::BattleParticipant *> participants;
        std::vector<bool> enemy;
        participants.reserve(chars_.size());
        enemy.reserve(chars_.size());
        for (auto &ci: chars_) {
            auto participant = std::make_unique<battle::BattleParticipant>(nullptr, ci.info);
            participants.push_back(participant.get());
            enemy.push_back(ci.side != 0);
            battleParticipants_.push_back(std::move(participant));
        }
        bool hasPlayer = false;
        bool hasEnemy = false;
        for (const auto value: enemy) {
            hasPlayer = hasPlayer || !value;
            hasEnemy = hasEnemy || value;
        }
        if (!hasPlayer || !hasEnemy) {
            discardBattleSession();
            chars_.clear();
            return false;
        }
        if (!battleEngine_.begin({std::move(participants), std::move(enemy),
                                  &battleRandom_, std::move(nextInventory)})) {
            discardBattleSession();
            chars_.clear();
            return false;
        }
        battleBag_ = std::move(nextBattleBag);
        battleBagActive_ = true;
    } catch (const std::bad_alloc &) {
        discardBattleSession();
        chars_.clear();
        return false;
    }

    for (auto &cell: cellInfo_) {
        cell.charInfo = nullptr;
        cell.insideMovingArea = 0;
    }
    for (auto &ci: chars_) {
        auto &cell = cellInfo_[static_cast<std::size_t>(ci.y) * static_cast<std::size_t>(mapWidth_) + ci.x];
        cell.charInfo = &ci;
    }
    turnOrder_.reserve(chars_.size());
    for (auto &ci: chars_) {
        turnOrder_.emplace_back(&ci);
    }
    recalcKnowledge();
    frameUpdate();
    pendingBattleMusic_ = info->music;
    return true;
}

}
