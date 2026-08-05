#include "mapwithevent.hh"

#include "logic/rle.hh"
#include "content/constants.hh"
#include "content/event.hh"
#include "content/grpdata.hh"
#include "world/action.hh"
#include "world/bag.hh"
#include "world/savedata.hh"
#include "world/submap.hh"
#include "world/strings.hh"
#include "util/random.hh"

#include <fmt/xchar.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hojy::scene {

namespace {

struct TextureCatalog final {
    std::vector<bool> valid;
};

std::optional<TextureCatalog> loadTextureCatalog(std::int16_t subMapId) {
    if (subMapId < 0) { return std::nullopt; }
    ::hojy::content::GrpData::DataSet textures;
    if (!::hojy::content::GrpData::loadData("SDX", "SMP", textures)
        && !::hojy::content::GrpData::loadData(
            fmt::format("SDX{:03}", subMapId),
            fmt::format("SMP{:03}", subMapId), textures)) {
        return std::nullopt;
    }
    constexpr std::size_t MaxTextureEntries = 32768U;
    if (textures.empty() || textures.size() > MaxTextureEntries) {
        return std::nullopt;
    }
    TextureCatalog catalog;
    try {
        catalog.valid.assign(textures.size(), false);
        for (std::size_t index = 0; index < textures.size(); ++index) {
            if (!textures[index].empty()
                && !logic::validateRleData(textures[index])) {
                return std::nullopt;
            }
            catalog.valid[index] = !textures[index].empty();
        }
    } catch (const std::bad_alloc &) {
        return std::nullopt;
    }
    return catalog;
}

}

bool MapWithEvent::validateSubMapStateCandidate(
        std::int16_t subMapId,
        const ::hojy::world::state::SubMapEventData &events,
        const std::vector<std::int16_t> &eventLayer,
        logic::SubMapStateSnapshot &snapshot) const {
    if (subMapId < 0
        || eventLayer.size() != static_cast<std::size_t>(
            ::hojy::content::SubMapWidth * ::hojy::content::SubMapHeight)) {
        return false;
    }
    std::array<logic::SubMapEventRecord, logic::SubMapEventCount> records{};
    for (std::size_t index = 0; index < records.size(); ++index) {
        const auto &event = events.events[index];
        records[index] = logic::SubMapEventRecord{
            event.blocked, event.index, event.currTex, event.endTex,
            event.begTex, event.texDelay, event.x, event.y,
        };
    }
    const bool localTextureSet = subMapId == subMapId_ && !texData_.empty();
    std::optional<TextureCatalog> remoteCatalog;
    if (!localTextureSet) {
        remoteCatalog = loadTextureCatalog(subMapId);
        if (!remoteCatalog) { return false; }
    }
    const auto textureCount = localTextureSet
        ? texData_.size() : remoteCatalog->valid.size();
    const auto textureValid = [this, localTextureSet,
                               &remoteCatalog](std::int16_t encoded) {
        if (encoded < 0) { return encoded == -1; }
        const auto id = static_cast<std::size_t>(encoded >> 1);
        if (localTextureSet) {
            return id < texData_.size() && !texData_[id].empty()
                && logic::validateRleData(texData_[id]);
        }
        return remoteCatalog && id < remoteCatalog->valid.size()
            && remoteCatalog->valid[id];
    };
    return logic::buildSubMapStateSnapshot(
        records, eventLayer, ::hojy::content::SubMapWidth,
        ::hojy::content::SubMapHeight, textureCount, textureValid, snapshot);
}

}
