#pragma once

#include "data/grpdata.hh"

#include <array>
#include <cstdint>
#include <cstring>
#include <set>
#include <string>
#include <utility>

namespace hojy::scene::detail {

struct WarfieldTextureLoad {
    data::GrpData::DataSet textures;
    bool shared = false;
    std::uint16_t cellWidth = 0;
    std::uint16_t cellHeight = 0;
    std::uint16_t offsetX = 0;
    std::uint16_t offsetY = 0;
};

inline const std::string &warfieldTextureAt(
    const data::GrpData::DataSet &textures, int index) {
    static const std::string empty;
    if (index < 0 || static_cast<std::size_t>(index) >= textures.size()) {
        return empty;
    }
    return textures[static_cast<std::size_t>(index)];
}

inline bool validateWarfieldTextureIds(
    const std::int16_t *earthLayer, const std::int16_t *buildingLayer,
    std::size_t cellCount, std::size_t textureCount) {
    if (!earthLayer || !buildingLayer || textureCount == 0) { return false; }
    for (std::size_t i = 0; i < cellCount; ++i) {
        if (earthLayer[i] < 0 || buildingLayer[i] < 0) { return false; }
        const auto earthId = static_cast<std::size_t>(earthLayer[i] >> 1);
        const auto buildingId = static_cast<std::size_t>(buildingLayer[i] >> 1);
        if (earthId >= textureCount || buildingId >= textureCount) {
            return false;
        }
    }
    return true;
}

inline bool readWarfieldTextureHeader(
    const data::GrpData::DataSet &textures,
    WarfieldTextureLoad &result) {
    if (textures.empty() || textures[0].size() < sizeof(std::uint16_t) * 4) {
        return false;
    }
    std::array<std::uint16_t, 4> values{};
    std::memcpy(values.data(), textures[0].data(), sizeof(values));
    if (values[0] < 2 || values[1] < 2) { return false; }
    result.cellWidth = values[0];
    result.cellHeight = values[1];
    result.offsetX = values[2];
    result.offsetY = values[3];
    return true;
}

template<typename Loader>
bool loadWarfieldTextures(const std::string &specificIndex,
                          const std::string &specificGroup,
                          Loader &&loader,
                          WarfieldTextureLoad &result) {
    data::GrpData::DataSet textures;
    const bool shared = loader("WDX", "WMP", textures);
    if (!shared) {
        textures.clear();
        if (!loader(specificIndex, specificGroup, textures)) {
            return false;
        }
    }

    WarfieldTextureLoad candidate;
    if (!readWarfieldTextureHeader(textures, candidate)) { return false; }
    candidate.textures = std::move(textures);
    candidate.shared = shared;
    result = std::move(candidate);
    return true;
}

inline void commitWarfieldTextureCache(std::set<std::int16_t> &loadedMaps,
                                       std::int16_t warMapId,
                                       bool shared) {
    loadedMaps.clear();
    if (shared) {
        for (std::int16_t id = 0; id < 1000; ++id) {
            loadedMaps.insert(id);
        }
        return;
    }
    loadedMaps.insert(warMapId);
}

}
