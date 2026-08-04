#include "scene/warfield_load.hh"
#include "test_support.hh"

#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

hojy::content::GrpData::DataSet makeTextures(
    std::uint16_t cellWidth, std::uint16_t cellHeight,
    std::uint16_t offsetX, std::uint16_t offsetY) {
    std::uint16_t values[] = {cellWidth, cellHeight, offsetX, offsetY};
    hojy::content::GrpData::DataSet textures(1);
    textures[0].resize(sizeof(values));
    std::memcpy(textures[0].data(), values, sizeof(values));
    return textures;
}

void testInvalidTextureHeaderLeavesPreviousResultUntouched() {
    hojy::scene::detail::WarfieldTextureLoad result;
    result.textures = {"keep"};
    result.cellWidth = 48;
    result.cellHeight = 24;

    const auto loaded = hojy::scene::detail::loadWarfieldTextures(
        "WDX007", "WMP007",
        [](const std::string &, const std::string &,
           hojy::content::GrpData::DataSet &textures) {
            textures = {std::string(7, '\0')};
            return true;
        },
        result);

    HOJY_CHECK_EQ(loaded, false);
    HOJY_CHECK_EQ(result.textures.size(), 1U);
    HOJY_CHECK_EQ(result.textures[0], std::string("keep"));
    HOJY_CHECK_EQ(result.cellWidth, std::uint16_t(48));
    HOJY_CHECK_EQ(result.cellHeight, std::uint16_t(24));
}

void testSpecificTextureFallbackCommitsOnlyValidatedData() {
    std::vector<std::pair<std::string, std::string>> calls;
    hojy::scene::detail::WarfieldTextureLoad result;

    const auto loaded = hojy::scene::detail::loadWarfieldTextures(
        "WDX007", "WMP007",
        [&calls](const std::string &idx, const std::string &grp,
                 hojy::content::GrpData::DataSet &textures) {
            calls.emplace_back(idx, grp);
            if (idx == "WDX") { return false; }
            textures = makeTextures(48, 24, 12, 6);
            return true;
        },
        result);

    HOJY_CHECK_EQ(loaded, true);
    HOJY_CHECK_EQ(calls.size(), 2U);
    HOJY_CHECK_EQ(calls[0].first, std::string("WDX"));
    HOJY_CHECK_EQ(calls[1].first, std::string("WDX007"));
    HOJY_CHECK_EQ(result.shared, false);
    HOJY_CHECK_EQ(result.cellWidth, std::uint16_t(48));
    HOJY_CHECK_EQ(result.cellHeight, std::uint16_t(24));
    HOJY_CHECK_EQ(result.offsetX, std::uint16_t(12));
    HOJY_CHECK_EQ(result.offsetY, std::uint16_t(6));
}

void testZeroCellDimensionsAreRejectedWithoutMutation() {
    hojy::scene::detail::WarfieldTextureLoad result;
    result.textures = {"keep"};
    result.cellWidth = 48;
    result.cellHeight = 24;

    const auto loaded = hojy::scene::detail::loadWarfieldTextures(
        "WDX007", "WMP007",
        [](const std::string &, const std::string &,
           hojy::content::GrpData::DataSet &textures) {
            textures = makeTextures(0, 0, 0, 0);
            return true;
        },
        result);

    HOJY_CHECK_EQ(loaded, false);
    HOJY_CHECK_EQ(result.textures[0], std::string("keep"));
    HOJY_CHECK_EQ(result.cellWidth, std::uint16_t(48));
    HOJY_CHECK_EQ(result.cellHeight, std::uint16_t(24));
}

void testSpecificTextureCacheOnlyMarksCurrentMap() {
    std::set<std::int16_t> loadedMaps = {1};

    hojy::scene::detail::commitWarfieldTextureCache(loadedMaps, 7, false);

    HOJY_CHECK_EQ(loadedMaps.size(), 1U);
    HOJY_CHECK_EQ(loadedMaps.count(1), 0U);
    HOJY_CHECK_EQ(loadedMaps.count(7), 1U);
}

void testTextureLookupRejectsInvalidIndices() {
    const hojy::content::GrpData::DataSet textures = {"earth", "building"};
    HOJY_CHECK_EQ(hojy::scene::detail::warfieldTextureAt(textures, 0),
                  std::string("earth"));
    HOJY_CHECK_EQ(hojy::scene::detail::warfieldTextureAt(textures, -1),
                  std::string());
    HOJY_CHECK_EQ(hojy::scene::detail::warfieldTextureAt(textures, 2),
                  std::string());
}

void testLayerTextureIdsMustFitLoadedTextureSet() {
    const std::int16_t earth[] = {0, 2, 6};
    const std::int16_t building[] = {0, 2, 0};
    HOJY_CHECK_EQ(hojy::scene::detail::validateWarfieldTextureIds(
                      earth, building, 3, 3), false);
    HOJY_CHECK_EQ(hojy::scene::detail::validateWarfieldTextureIds(
                      earth, building, 3, 5), true);

    const std::int16_t invalid[] = {-2, 0, 0};
    HOJY_CHECK_EQ(hojy::scene::detail::validateWarfieldTextureIds(
        invalid, building, 3, 5), false);
}

void testWarfieldRosterRejectsDuplicateCharacterIds() {
    HOJY_CHECK_EQ(
        hojy::scene::detail::validateUniqueWarfieldCharacterIds({1, 2, 3}),
        true);
    HOJY_CHECK_EQ(
        hojy::scene::detail::validateUniqueWarfieldCharacterIds({1, 2, 1}),
        false);
    HOJY_CHECK_EQ(
        hojy::scene::detail::validateUniqueWarfieldCharacterIds({-1, -1, 4}),
        true);
}

}

int main() {
    try {
        testInvalidTextureHeaderLeavesPreviousResultUntouched();
        testSpecificTextureFallbackCommitsOnlyValidatedData();
        testZeroCellDimensionsAreRejectedWithoutMutation();
        testSpecificTextureCacheOnlyMarksCurrentMap();
        testTextureLookupRejectsInvalidIndices();
        testLayerTextureIdsMustFitLoadedTextureSet();
        testWarfieldRosterRejectsDuplicateCharacterIds();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
