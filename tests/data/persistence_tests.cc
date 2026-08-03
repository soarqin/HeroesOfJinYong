/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>
 */

#include "data/event.hh"
#include "data/grpdata.hh"
#include "data/warfielddata.hh"
#include "mem/bag.hh"
#include "mem/savedata.hh"
#include "mem/serializable.hh"
#include "mem/strings.hh"
#include "test_support.hh"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

class ScopedTempDirectory {
public:
    ScopedTempDirectory(): oldPath_(std::filesystem::current_path()) {
        const auto suffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() / ("hojy-persistence-" + std::to_string(suffix));
        std::filesystem::create_directories(path_);
        std::filesystem::current_path(path_);
    }

    ~ScopedTempDirectory() {
        std::error_code ec;
        std::filesystem::current_path(oldPath_, ec);
        std::filesystem::remove_all(path_, ec);
    }

private:
    std::filesystem::path oldPath_, path_;
};

template<typename T>
std::string asBytes(const T &value) {
    return {reinterpret_cast<const char *>(&value), sizeof(value)};
}

void writeBytes(const std::string &filename, const std::string &data) {
    std::ofstream file(filename, std::ios::binary);
    file.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!file) { throw std::runtime_error("failed to write " + filename); }
}

void writeOffsets(const std::string &filename, const std::vector<std::uint32_t> &offsets) {
    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<const char *>(offsets.data()),
               static_cast<std::streamsize>(offsets.size() * sizeof(std::uint32_t)));
    if (!file) { throw std::runtime_error("failed to write " + filename); }
}

void serializableRejectsInvalidSizes() {
    hojy::mem::SerializableStruct<std::uint32_t> scalar;
    const std::uint32_t value = 0x12345678;
    HOJY_CHECK_EQ(scalar.deserialize(asBytes(value)), true);
    HOJY_CHECK_EQ(*scalar.operator->(), value);
    HOJY_CHECK_EQ(scalar.deserialize(std::string(sizeof(value) - 1, '\0')), false);
    HOJY_CHECK_EQ(*scalar.operator->(), value);

    hojy::mem::SerializableStructVec<std::uint16_t> values;
    const std::uint16_t initial[] = {7, 9};
    HOJY_CHECK_EQ(values.deserialize(std::string(reinterpret_cast<const char *>(initial), sizeof(initial))), true);
    HOJY_CHECK_EQ(values.size(), 2U);
    HOJY_CHECK_EQ(values.deserialize(std::string(3, '\0')), false);
    HOJY_CHECK_EQ(values.size(), 2U);
}

void grpDataValidatesOffsets() {
    using hojy::data::GrpData;
    const GrpData::DataSet expected = {"ab", "", "cde"};
    HOJY_CHECK_EQ(GrpData::saveData("GOOD", expected), true);
    GrpData::DataSet loaded;
    HOJY_CHECK_EQ(GrpData::loadData("GOOD", loaded), true);
    HOJY_CHECK_EQ(loaded, expected);

    loaded = {"unchanged"};
    writeBytes("BAD_ALIGN.IDX", "abc");
    writeBytes("BAD_ALIGN.GRP", "abc");
    HOJY_CHECK_EQ(GrpData::loadData("BAD_ALIGN", loaded), false);
    HOJY_CHECK_EQ(loaded, GrpData::DataSet{"unchanged"});

    writeOffsets("BACKWARD.IDX", {2, 1});
    writeBytes("BACKWARD.GRP", "ab");
    HOJY_CHECK_EQ(GrpData::loadData("BACKWARD", loaded), false);
    HOJY_CHECK_EQ(loaded, GrpData::DataSet{"unchanged"});

    writeOffsets("PAST_END.IDX", {3});
    writeBytes("PAST_END.GRP", "ab");
    HOJY_CHECK_EQ(GrpData::loadData("PAST_END", loaded), false);
    HOJY_CHECK_EQ(loaded, GrpData::DataSet{"unchanged"});

    writeOffsets("TRAILING_DATA.IDX", {1});
    writeBytes("TRAILING_DATA.GRP", "ab");
    HOJY_CHECK_EQ(GrpData::loadData("TRAILING_DATA", loaded), false);
    HOJY_CHECK_EQ(loaded, GrpData::DataSet{"unchanged"});

    writeBytes("EMPTY_INDEX.IDX", "");
    writeBytes("EMPTY_INDEX.GRP", "ab");
    HOJY_CHECK_EQ(GrpData::loadData("EMPTY_INDEX", loaded), false);
    HOJY_CHECK_EQ(loaded, GrpData::DataSet{"unchanged"});

    writeOffsets("ZERO_END.IDX", {0});
    writeBytes("ZERO_END.GRP", "abc");
    HOJY_CHECK_EQ(GrpData::loadData("ZERO_END", loaded), true);
    HOJY_CHECK_EQ(loaded, GrpData::DataSet{"abc"});

    writeOffsets("ZERO_PADDING.IDX", {3, 0, 0});
    writeBytes("ZERO_PADDING.GRP", "abc");
    HOJY_CHECK_EQ(GrpData::loadData("ZERO_PADDING", loaded), true);
    HOJY_CHECK_EQ(loaded, (GrpData::DataSet{"abc", "", ""}));

    writeOffsets("ZERO_MIDDLE.IDX", {0, 3});
    writeBytes("ZERO_MIDDLE.GRP", "abc");
    HOJY_CHECK_EQ(GrpData::loadData("ZERO_MIDDLE", loaded), false);
    HOJY_CHECK_EQ(loaded, (GrpData::DataSet{"abc", "", ""}));
    HOJY_CHECK_EQ(GrpData::saveData("LEADING_EMPTY", {"", "abc"}), false);
    HOJY_CHECK_EQ(std::filesystem::exists("LEADING_EMPTY.IDX"), false);
}

void eventRejectsOddBytecode() {
    using hojy::data::GrpData;
    const std::int16_t bytecode[] = {1, 2};
    HOJY_CHECK_EQ(GrpData::saveData("EVENT", {std::string(reinterpret_cast<const char *>(bytecode), sizeof(bytecode))}), true);
    hojy::data::Event event;
    HOJY_CHECK_EQ(event.loadEvent("EVENT"), true);
    HOJY_CHECK_EQ(event.event(0).size(), 2U);

    HOJY_CHECK_EQ(GrpData::saveData("EVENT", {std::string(3, '\1')}), true);
    HOJY_CHECK_EQ(event.loadEvent("EVENT"), false);
    HOJY_CHECK_EQ(event.event(0).size(), 2U);
    HOJY_CHECK_EQ(event.event(0)[0], 1);
}

void warfieldDataAcceptsSharedPartialMaps() {
    using hojy::data::GrpData;
    std::array<hojy::data::WarfieldInfo, 2> info{};
    info[0].id = 10;
    info[0].warFieldId = 0;
    info[1].id = 11;
    info[1].warFieldId = 0;
    writeBytes("WAR.STA", std::string(reinterpret_cast<const char *>(info.data()), sizeof(info)));

    std::vector<std::int16_t> map(hojy::data::WarFieldWidth * hojy::data::WarFieldHeight, 7);
    HOJY_CHECK_EQ(GrpData::saveData("WARFLD", {
        std::string(reinterpret_cast<const char *>(map.data()), map.size() * sizeof(std::int16_t))
    }), true);

    hojy::data::WarfieldData warfields;
    HOJY_CHECK_EQ(warfields.load("WAR.STA", "WARFLD"), true);
    HOJY_CHECK_EQ(warfields.size(), 2U);
    HOJY_CHECK_EQ(warfields.layers(0)->layers[0][0], 7);
    HOJY_CHECK_EQ(warfields.layers(0)->layers[1][0], 0);

    info[1].warFieldId = 1;
    writeBytes("WAR.STA", std::string(reinterpret_cast<const char *>(info.data()), sizeof(info)));
    HOJY_CHECK_EQ(warfields.load("WAR.STA", "WARFLD"), false);
    HOJY_CHECK_EQ(warfields.size(), 2U);
    HOJY_CHECK_EQ(warfields.info(1)->warFieldId, 0);

    info[1].warFieldId = 0;
    writeBytes("WAR.STA", std::string(reinterpret_cast<const char *>(info.data()), sizeof(info)));
    HOJY_CHECK_EQ(GrpData::saveData("WARFLD", {std::string(sizeof(std::int16_t), '\0')}), true);
    HOJY_CHECK_EQ(warfields.load("WAR.STA", "WARFLD"), false);
    HOJY_CHECK_EQ(warfields.layers(0)->layers[0][0], 7);

    HOJY_CHECK_EQ(GrpData::saveData("WARFLD", {""}), true);
    HOJY_CHECK_EQ(warfields.load("WAR.STA", "WARFLD"), false);
    HOJY_CHECK_EQ(warfields.layers(0)->layers[0][0], 7);
}

void stringsRequireAllUiEntries() {
    std::string content = "strings = [\n";
    for (int i = 0; i < 138; ++i) {
        content += "    \"text" + std::to_string(i) + "\",\n";
    }
    content += "]\n";
    writeBytes("strings.toml", content);

    hojy::mem::Strings strings;
    HOJY_CHECK_EQ(strings.load("strings.toml"), true);
    HOJY_CHECK_EQ(strings(hojy::mem::Strings::Text, 137), L"text137");

    writeBytes("strings.toml", "strings = [\"truncated\"]\n");
    HOJY_CHECK_EQ(strings.load("strings.toml"), false);
    HOJY_CHECK_EQ(strings(hojy::mem::Strings::Text, 137), L"text137");
}

hojy::mem::SaveData makeSaveData(std::int16_t mainX, std::int16_t itemId, std::int16_t itemCount) {
    hojy::mem::SaveData data;
    auto &base = *data.baseInfo.operator->();
    base.mainX = mainX;
    for (auto &member: base.members) { member = -1; }
    for (auto &item: base.items) { item = {-1, 0}; }
    base.items[0] = {itemId, itemCount};

    hojy::mem::SubMapData subMap{};
    HOJY_CHECK_EQ(data.subMapInfo.deserialize(asBytes(subMap)), true);
    data.subMapLayerInfo.resize(1);
    data.subMapEventInfo.resize(1);
    return data;
}

void writeSaveSlot(int slot, hojy::mem::SaveData &data, bool writeEvents, size_t layerCount = 1) {
    using hojy::data::GrpData;
    GrpData::DataSet ranger(6);
    data.baseInfo.serialize(ranger[0]);
    data.charInfo.serialize(ranger[1]);
    data.itemInfo.serialize(ranger[2]);
    data.subMapInfo.serialize(ranger[3]);
    data.skillInfo.serialize(ranger[4]);
    data.shopInfo.serialize(ranger[5]);
    HOJY_CHECK_EQ(GrpData::saveData("R" + std::to_string(slot), ranger, true), true);

    GrpData::DataSet layers(layerCount);
    for (size_t i = 0; i < layerCount; ++i) {
        data.subMapLayerInfo[0].serialize(layers[i]);
    }
    HOJY_CHECK_EQ(GrpData::saveData("S" + std::to_string(slot), layers, true), true);

    if (writeEvents) {
        GrpData::DataSet events(1);
        data.subMapEventInfo[0].serialize(events[0]);
        HOJY_CHECK_EQ(GrpData::saveData("D" + std::to_string(slot), events, true), true);
    }
}

void saveDataLoadsTransactionally() {
    auto first = makeSaveData(111, 1, 5);
    writeSaveSlot(1, first, true);
    HOJY_CHECK_EQ(hojy::mem::gSaveData.load(1), true);
    HOJY_CHECK_EQ(hojy::mem::gSaveData.baseInfo->mainX, 111);
    HOJY_CHECK_EQ(hojy::mem::gBag[1], 5);

    auto missingEvents = makeSaveData(222, 2, 9);
    writeSaveSlot(2, missingEvents, false);
    HOJY_CHECK_EQ(hojy::mem::gSaveData.load(2), false);
    HOJY_CHECK_EQ(hojy::mem::gSaveData.baseInfo->mainX, 111);
    HOJY_CHECK_EQ(hojy::mem::gSaveData.subMapLayerInfo.size(), 1U);
    HOJY_CHECK_EQ(hojy::mem::gSaveData.subMapEventInfo.size(), 1U);
    HOJY_CHECK_EQ(hojy::mem::gBag[1], 5);
    HOJY_CHECK_EQ(hojy::mem::gBag[2], 0);

    auto extraLayer = makeSaveData(333, 3, 7);
    writeSaveSlot(2, extraLayer, true, 2);
    HOJY_CHECK_EQ(hojy::mem::gSaveData.load(2), false);
    HOJY_CHECK_EQ(hojy::mem::gSaveData.baseInfo->mainX, 111);
    HOJY_CHECK_EQ(hojy::mem::gBag[1], 5);
}

void saveRejectsMismatchedCollections() {
    hojy::mem::gSaveData.subMapLayerInfo.resize(2);
    hojy::mem::gSaveData.subMapEventInfo.resize(1);
    HOJY_CHECK_EQ(hojy::mem::gSaveData.save(3), false);
    HOJY_CHECK_EQ(std::filesystem::exists("R3.IDX"), false);

    const std::array<hojy::mem::SubMapData, 2> subMaps{};
    HOJY_CHECK_EQ(hojy::mem::gSaveData.subMapInfo.deserialize(
        std::string(reinterpret_cast<const char *>(subMaps.data()), sizeof(subMaps))), true);
    hojy::mem::gSaveData.subMapEventInfo.resize(2);
    HOJY_CHECK_EQ(hojy::mem::gSaveData.save(3), true);
    HOJY_CHECK_EQ(std::filesystem::file_size("D3.IDX"), sizeof(std::uint32_t) * 2);
}

}

int main() {
    try {
        ScopedTempDirectory tempDirectory;
        serializableRejectsInvalidSizes();
        grpDataValidatesOffsets();
        eventRejectsOddBytecode();
        warfieldDataAcceptsSharedPartialMaps();
        stringsRequireAllUiEntries();
        saveDataLoadsTransactionally();
        saveRejectsMismatchedCollections();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
