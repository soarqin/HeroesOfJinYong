/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "content/atomic_file.hh"
#include "makedata_assets.hh"

#include <array>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <new>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace {

namespace fs = std::filesystem;

using DataSet = std::vector<std::string>;
using SourceFiles = std::map<std::string, fs::path>;

constexpr std::array<const char *, 37> DataFiles = {
    "ALLDEF.GRP",
    "ALLDEF.IDX",
    "ALLSIN.GRP",
    "ALLSIN.IDX",
    "BUILDING.002",
    "BUILDX.002",
    "BUILDY.002",
    "EARTH.002",
    "SURFACE.002",
    "CLOUD.GRP",
    "CLOUD.IDX",
    "DEAD.BIG",
    "EFT.GRP",
    "EFT.IDX",
    "ENDCOL.COL",
    "ENDWORD.GRP",
    "ENDWORD.IDX",
    "KEND.GRP",
    "KEND.IDX",
    "HDGRP.GRP",
    "HDGRP.IDX",
    "KDEF.GRP",
    "KDEF.IDX",
    "MMAP.COL",
    "MMAP.GRP",
    "MMAP.IDX",
    "RANGER.GRP",
    "RANGER.IDX",
    "TALK.GRP",
    "TALK.IDX",
    "TITLE.BIG",
    "TITLE.GRP",
    "TITLE.IDX",
    "WAR.STA",
    "WARFLD.GRP",
    "WARFLD.IDX",
    "Z.DAT",
};

std::string upperAscii(std::string value) {
    for (char &ch : value) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return value;
}

std::string numberedName(const char *prefix, int number, const char *suffix = "") {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%s%03d%s", prefix, number, suffix);
    return buffer;
}

bool readFile(const fs::path &path, std::string &data, std::string &error) {
    std::error_code ec;
    const auto fileSize = fs::file_size(path, ec);
    if (ec) {
        error = "cannot get file size: " + path.string() + ": " + ec.message();
        return false;
    }
    if (fileSize > std::numeric_limits<std::size_t>::max()
        || fileSize > static_cast<std::uintmax_t>(std::string().max_size())
        || fileSize > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max())) {
        error = "file is too large: " + path.string();
        return false;
    }

    try {
        std::string loaded(static_cast<std::size_t>(fileSize), '\0');
        std::ifstream input(path, std::ios::binary);
        if (!input) {
            error = "cannot open file: " + path.string();
            return false;
        }
        if (!loaded.empty()) {
            input.read(loaded.data(), static_cast<std::streamsize>(loaded.size()));
            if (input.gcount() != static_cast<std::streamsize>(loaded.size())) {
                error = "short read: " + path.string();
                return false;
            }
        }
        data = std::move(loaded);
        return true;
    } catch (const std::bad_alloc &) {
        error = "not enough memory to read: " + path.string();
        return false;
    }
}

std::uint32_t readUint32LE(const char *data) {
    const auto *bytes = reinterpret_cast<const unsigned char *>(data);
    return static_cast<std::uint32_t>(bytes[0])
        | (static_cast<std::uint32_t>(bytes[1]) << 8U)
        | (static_cast<std::uint32_t>(bytes[2]) << 16U)
        | (static_cast<std::uint32_t>(bytes[3]) << 24U);
}

void appendUint32LE(std::string &data, std::uint32_t value) {
    data.push_back(static_cast<char>(value & 0xffU));
    data.push_back(static_cast<char>((value >> 8U) & 0xffU));
    data.push_back(static_cast<char>((value >> 16U) & 0xffU));
    data.push_back(static_cast<char>((value >> 24U) & 0xffU));
}

bool loadGrp(const fs::path &indexPath, const fs::path &groupPath,
             DataSet &dataSet, std::string &error) {
    std::string indexData;
    std::string groupData;
    if (!readFile(indexPath, indexData, error) || !readFile(groupPath, groupData, error)) {
        return false;
    }
    if (indexData.size() % sizeof(std::uint32_t) != 0) {
        error = "invalid IDX length: " + indexPath.string();
        return false;
    }
    if (groupData.size() > std::numeric_limits<std::uint32_t>::max()) {
        error = "GRP file is too large: " + groupPath.string();
        return false;
    }
    const auto count = indexData.size() / sizeof(std::uint32_t);
    if (count > DataSet().max_size()) {
        error = "IDX has too many entries: " + indexPath.string();
        return false;
    }

    try {
        DataSet loaded(count);
        std::uint32_t offset = 0;
        bool reachedEnd = false;
        for (std::size_t i = 0; i < count; ++i) {
            std::uint32_t endOffset = readUint32LE(indexData.data() + i * sizeof(std::uint32_t));
            if (endOffset == 0) {
                reachedEnd = true;
                endOffset = static_cast<std::uint32_t>(groupData.size());
            } else if (reachedEnd) {
                error = "non-zero IDX offset follows an end marker: " + indexPath.string();
                return false;
            }
            if (endOffset < offset || endOffset > groupData.size()) {
                error = "IDX offset is outside the GRP file: " + indexPath.string();
                return false;
            }
            const auto size = static_cast<std::size_t>(endOffset - offset);
            if (size != 0) {
                loaded[i].assign(groupData.data() + offset, size);
            }
            offset = endOffset;
        }
        if (offset != groupData.size()) {
            error = "IDX does not consume the complete GRP file: " + indexPath.string();
            return false;
        }
        dataSet = std::move(loaded);
        return true;
    } catch (const std::bad_alloc &) {
        error = "not enough memory to load map files: " + indexPath.string();
        return false;
    }
}

bool encodeGrp(const DataSet &dataSet, std::string &indexData,
               std::string &groupData, std::string &error) {
    try {
        std::uint64_t totalSize = 0;
        for (std::size_t i = 0; i < dataSet.size(); ++i) {
            if (dataSet[i].size() > std::numeric_limits<std::uint64_t>::max() - totalSize) {
                error = "merged GRP size overflows";
                return false;
            }
            totalSize += dataSet[i].size();
            if (totalSize > std::numeric_limits<std::uint32_t>::max()) {
                error = "merged GRP exceeds the 32-bit IDX limit";
                return false;
            }
            if (totalSize == 0 && i + 1 != dataSet.size()) {
                error = "merged map contains an unsupported leading empty entry";
                return false;
            }
        }
        if (dataSet.size() > std::string().max_size() / sizeof(std::uint32_t)) {
            error = "merged IDX is too large";
            return false;
        }

        std::string newIndex;
        std::string newGroup;
        newIndex.reserve(dataSet.size() * sizeof(std::uint32_t));
        newGroup.reserve(static_cast<std::size_t>(totalSize));
        std::uint32_t offset = 0;
        for (const auto &entry : dataSet) {
            offset += static_cast<std::uint32_t>(entry.size());
            appendUint32LE(newIndex, offset);
            newGroup.append(entry);
        }
        indexData = std::move(newIndex);
        groupData = std::move(newGroup);
        return true;
    } catch (const std::bad_alloc &) {
        error = "not enough memory to encode merged map files";
        return false;
    }
}

bool collectSourceFiles(const fs::path &source, SourceFiles &files, std::string &error) {
    std::error_code ec;
    fs::directory_iterator iterator(source, ec), end;
    if (ec) {
        error = "cannot enumerate source directory: " + source.string() + ": " + ec.message();
        return false;
    }
    while (iterator != end) {
        const auto entry = *iterator;
        if (entry.is_regular_file(ec)) {
            const auto key = upperAscii(entry.path().filename().string());
            if (!files.emplace(key, entry.path()).second) {
                error = "source contains duplicate case-insensitive file names: " + key;
                return false;
            }
        } else if (ec) {
            error = "cannot inspect source entry: " + entry.path().string() + ": " + ec.message();
            return false;
        }
        iterator.increment(ec);
        if (ec) {
            error = "cannot enumerate source directory: " + source.string() + ": " + ec.message();
            return false;
        }
    }
    return true;
}

bool mergeMaps(const SourceFiles &files, const char *indexPrefix, const char *groupPrefix,
               DataSet &merged, bool &found, std::string &error) {
    DataSet result;
    found = false;
    for (int i = 0; i < 1000; ++i) {
        const auto indexName = numberedName(indexPrefix, i);
        const auto groupName = numberedName(groupPrefix, i);
        const auto index = files.find(indexName);
        const auto group = files.find(groupName);
        if ((index == files.end()) != (group == files.end())) {
            error = "incomplete map pair: " + indexName + " / " + groupName;
            return false;
        }
        if (index == files.end()) {
            continue;
        }

        DataSet single;
        if (!loadGrp(index->second, group->second, single, error)) {
            return false;
        }
        std::fprintf(stdout, "loaded %s %s\n", index->second.string().c_str(),
                     group->second.string().c_str());
        found = true;
        if (single.size() > result.size()) {
            result.resize(single.size());
        }
        for (std::size_t j = 0; j < single.size(); ++j) {
            if (single[j].empty()) {
                continue;
            }
            if (result[j].empty()) {
                result[j] = std::move(single[j]);
                continue;
            }
            if (result[j].size() != single[j].size()) {
                std::fprintf(stderr, "warning: map size mismatch: %d %zu %zu != %zu\n",
                             i, j, result[j].size(), single[j].size());
            }
        }
    }

    if (!found) {
        const auto index = files.find(indexPrefix);
        const auto group = files.find(groupPrefix);
        if ((index == files.end()) != (group == files.end())) {
            error = std::string("incomplete merged map pair: ") + indexPrefix + " / " + groupPrefix;
            return false;
        }
        if (index != files.end()) {
            if (!loadGrp(index->second, group->second, result, error)) {
                return false;
            }
            found = true;
        }
    }

    merged = std::move(result);
    return true;
}

bool copyFile(const fs::path &source, const fs::path &destination, std::string &error) {
    std::error_code ec;
    if (fs::exists(destination, ec)) {
        ec.clear();
        if (fs::equivalent(source, destination, ec) && !ec) {
            return true;
        }
        ec.clear();
    }
    fs::copy_file(source, destination, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        error = "cannot copy " + source.string() + " to " + destination.string() + ": " + ec.message();
        return false;
    }
    return true;
}

bool copyKnownFile(const SourceFiles &files, const std::string &name,
                   const fs::path &dataDirectory, std::size_t &count,
                   bool required, std::string &error) {
    const auto file = files.find(name);
    if (file == files.end()) {
        if (required) {
            error = "required resource is missing: " + name;
            return false;
        }
        return true;
    }
    if (!copyFile(file->second, dataDirectory / name, error)) {
        return false;
    }
    ++count;
    return true;
}

bool copyOriginalResources(const SourceFiles &files, const fs::path &dataDirectory,
                           std::size_t &count, std::string &error) {
    count = 0;
    for (const auto *name : DataFiles) {
        if (!copyKnownFile(files, name, dataDirectory, count, true, error)) {
            return false;
        }
    }
    for (int i = 1; i <= 24; ++i) {
        char name[32];
        std::snprintf(name, sizeof(name), "GAME%02d.XMI", i);
        if (!copyKnownFile(files, name, dataDirectory, count, false, error)) {
            return false;
        }
    }
    for (int i = 0; i <= 23; ++i) {
        char name[32];
        std::snprintf(name, sizeof(name), "ATK%02d.WAV", i);
        if (!copyKnownFile(files, name, dataDirectory, count, false, error)) {
            return false;
        }
    }
    for (int i = 0; i <= 52; ++i) {
        char name[32];
        std::snprintf(name, sizeof(name), "E%02d.WAV", i);
        if (!copyKnownFile(files, name, dataDirectory, count, false, error)) {
            return false;
        }
    }
    for (int i = 0; i < 1000; ++i) {
        if (!copyKnownFile(files, numberedName("FIGHT", i, ".IDX"),
                           dataDirectory, count, false, error)
            || !copyKnownFile(files, numberedName("FIGHT", i, ".GRP"),
                              dataDirectory, count, false, error)) {
            return false;
        }
    }
    return true;
}

std::string trim(const std::string &value) {
    std::size_t first = 0;
    while (first < value.size() && std::isspace(static_cast<unsigned char>(value[first]))) {
        ++first;
    }
    std::size_t last = value.size();
    while (last > first && std::isspace(static_cast<unsigned char>(value[last - 1]))) {
        --last;
    }
    return value.substr(first, last - first);
}

std::string quoteToml(const std::string &value) {
    std::string result = "\"";
    for (const char ch : value) {
        switch (ch) {
        case '\\': result += "\\\\"; break;
        case '"': result += "\\\""; break;
        case '\b': result += "\\b"; break;
        case '\t': result += "\\t"; break;
        case '\n': result += "\\n"; break;
        case '\f': result += "\\f"; break;
        case '\r': result += "\\r"; break;
        default: result += ch; break;
        }
    }
    result += '"';
    return result;
}

bool makeConfig(const std::string &configTemplate, const std::string &fontPath,
                std::string &config, std::string &error) {
    const std::map<std::string, std::string> replacements = {
        {"data_path", "data_path = [\"data\"]"},
        {"music_path", "music_path = \"data\""},
        {"sound_path", "sound_path = \"data\""},
        {"save_path", "save_path = \"data\""},
        {"fonts", "fonts = " + quoteToml(fontPath)},
    };
    std::map<std::string, bool> replaced;
    for (const auto &entry : replacements) {
        replaced.emplace(entry.first, false);
    }

    std::string generated;
    bool inMain = false;
    std::size_t offset = 0;
    while (offset < configTemplate.size()) {
        const auto lineEnd = configTemplate.find('\n', offset);
        const auto length = lineEnd == std::string::npos
            ? configTemplate.size() - offset
            : lineEnd - offset;
        auto line = configTemplate.substr(offset, length);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const auto stripped = trim(line);
        if (!stripped.empty() && stripped.front() == '[') {
            inMain = stripped == "[main]";
        }

        bool didReplace = false;
        if (inMain) {
            const auto equal = stripped.find('=');
            if (equal != std::string::npos) {
                const auto key = trim(stripped.substr(0, equal));
                const auto replacement = replacements.find(key);
                if (replacement != replacements.end()) {
                    generated += replacement->second;
                    replaced[key] = true;
                    didReplace = true;
                }
            }
        }
        if (!didReplace) {
            generated += line;
        }
        if (lineEnd != std::string::npos) {
            generated += '\n';
            offset = lineEnd + 1;
        } else {
            offset = configTemplate.size();
        }
    }

    for (const auto &entry : replaced) {
        if (!entry.second) {
            error = "config template is missing main." + entry.first;
            return false;
        }
    }
    config = std::move(generated);
    return true;
}

bool writeMergedMap(const fs::path &dataDirectory, const char *indexName,
                    const char *groupName, const DataSet &dataSet, std::string &error) {
    std::string indexData;
    std::string groupData;
    if (!encodeGrp(dataSet, indexData, groupData, error)) {
        return false;
    }
    if (!hojy::content::AtomicFile::writePair({
            {dataDirectory / indexName, std::move(indexData)},
            {dataDirectory / groupName, std::move(groupData)},
        })) {
        error = std::string("cannot write merged map pair: ") + indexName + " / " + groupName;
        return false;
    }
    return true;
}

fs::path temporarySibling(const fs::path &path, const char *suffix) {
    const auto base = path.string() + suffix;
    std::error_code ec;
    for (unsigned int i = 0; i < 10000; ++i) {
        const auto candidate = base + std::to_string(i);
        if (!fs::exists(candidate, ec)) {
            return candidate;
        }
        ec.clear();
    }
    return {};
}

void removeTree(const fs::path &path) {
    std::error_code ec;
    fs::remove_all(path, ec);
}

bool installStagedOutput(const fs::path &stage, const fs::path &target,
                         std::string &error) {
    std::error_code ec;
    if (!fs::exists(target, ec)) {
        fs::rename(stage, target, ec);
        if (ec) {
            error = "cannot install generated target: " + ec.message();
            removeTree(stage);
            return false;
        }
        return true;
    }
    if (!fs::is_directory(target, ec)) {
        error = "target path is not a directory: " + target.string();
        return false;
    }

    const auto backup = temporarySibling(target, ".makedata-backup.");
    if (backup.empty()) {
        error = "cannot reserve a temporary backup path beside the target";
        return false;
    }
    fs::create_directories(backup, ec);
    if (ec) {
        error = "cannot create temporary backup directory: " + ec.message();
        return false;
    }

    constexpr std::array<const char *, 2> generatedEntries = {"data", "config.toml"};
    std::vector<std::pair<fs::path, fs::path>> backups;
    std::vector<fs::path> installed;
    auto rollback = [&]() {
        for (auto it = installed.rbegin(); it != installed.rend(); ++it) {
            std::error_code rollbackError;
            fs::remove_all(*it, rollbackError);
        }
        for (auto it = backups.rbegin(); it != backups.rend(); ++it) {
            std::error_code rollbackError;
            fs::rename(it->second, it->first, rollbackError);
        }
        removeTree(backup);
        removeTree(stage);
    };

    for (const auto *entry : generatedEntries) {
        const auto destination = target / entry;
        if (!fs::exists(destination, ec)) {
            ec.clear();
            continue;
        }
        const auto backupDestination = backup / entry;
        fs::rename(destination, backupDestination, ec);
        if (ec) {
            error = "cannot stage existing target entry: " + destination.string()
                + ": " + ec.message();
            rollback();
            return false;
        }
        backups.emplace_back(destination, backupDestination);
    }

    for (const auto *entry : generatedEntries) {
        const auto staged = stage / entry;
        const auto destination = target / entry;
        fs::rename(staged, destination, ec);
        if (ec) {
            error = "cannot install generated target entry: " + destination.string()
                + ": " + ec.message();
            rollback();
            return false;
        }
        installed.push_back(destination);
    }

    removeTree(backup);
    removeTree(stage);
    return true;
}

int run(int argc, char *argv[]) {
    if (argc != 4) {
        std::fprintf(stderr, "Usage: %s <original-game-path> <target-path> <font-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const fs::path source = argv[1];
    const fs::path target = argv[2];
    const fs::path font = argv[3];
    std::error_code ec;
    if (!fs::is_directory(source, ec)) {
        std::fprintf(stderr, "Source path is not a directory: %s\n", source.string().c_str());
        return EXIT_FAILURE;
    }
    ec.clear();
    if (!fs::is_regular_file(font, ec) || font.filename().empty()) {
        std::fprintf(stderr, "Font path is not a regular file: %s\n", font.string().c_str());
        return EXIT_FAILURE;
    }
    ec.clear();
    if (fs::exists(target, ec) && !fs::is_directory(target, ec)) {
        std::fprintf(stderr, "Target path is not a directory: %s\n", target.string().c_str());
        return EXIT_FAILURE;
    }

    SourceFiles files;
    std::string error;
    if (!collectSourceFiles(source, files, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return EXIT_FAILURE;
    }

    DataSet submaps;
    DataSet warfields;
    bool hasSubmaps = false;
    bool hasWarfields = false;
    if (!mergeMaps(files, "SDX", "SMP", submaps, hasSubmaps, error)
        || !mergeMaps(files, "WDX", "WMP", warfields, hasWarfields, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return EXIT_FAILURE;
    }
    if (!hasSubmaps || !hasWarfields) {
        error = !hasSubmaps
            ? "required SDX/SMP map files are missing"
            : "required WDX/WMP map files are missing";
        std::fprintf(stderr, "%s\n", error.c_str());
        return EXIT_FAILURE;
    }

    std::string generatedConfig;
    const auto relativeFont = (fs::path("data") / "font" / font.filename()).generic_string();
    if (!makeConfig(std::string(hojy::tools::assets::ConfigToml),
                    relativeFont, generatedConfig, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return EXIT_FAILURE;
    }

    const auto stage = temporarySibling(target, ".makedata-staging.");
    if (stage.empty()) {
        std::fprintf(stderr, "Cannot reserve a temporary staging directory beside the target.\n");
        return EXIT_FAILURE;
    }
    const auto dataDirectory = stage / "data";
    const auto fontDirectory = dataDirectory / "font";
    fs::create_directories(fontDirectory, ec);
    if (ec) {
        std::fprintf(stderr, "Cannot create target directories: %s\n", ec.message().c_str());
        removeTree(stage);
        return EXIT_FAILURE;
    }

    std::size_t copied = 0;
    if (!copyOriginalResources(files, dataDirectory, copied, error)
        || !copyFile(font, fontDirectory / font.filename(), error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        removeTree(stage);
        return EXIT_FAILURE;
    }
    if (!hojy::content::AtomicFile::write(
            dataDirectory / "strings.toml",
            std::string(hojy::tools::assets::StringsToml))) {
        std::fprintf(stderr, "Cannot write strings.toml to target data directory.\n");
        removeTree(stage);
        return EXIT_FAILURE;
    }
    if (hasSubmaps && !writeMergedMap(dataDirectory, "SDX", "SMP", submaps, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        removeTree(stage);
        return EXIT_FAILURE;
    }
    if (hasWarfields && !writeMergedMap(dataDirectory, "WDX", "WMP", warfields, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        removeTree(stage);
        return EXIT_FAILURE;
    }
    if (!hojy::content::AtomicFile::write(stage / "config.toml", generatedConfig)) {
        std::fprintf(stderr, "Cannot write config.toml to target directory.\n");
        removeTree(stage);
        return EXIT_FAILURE;
    }
    if (!installStagedOutput(stage, target, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return EXIT_FAILURE;
    }

    std::fprintf(stdout, "Copied %zu original resource files to %s\n", copied,
                 (target / "data").string().c_str());
    std::fprintf(stdout, "Generated %s\n", (target / "config.toml").string().c_str());
    return EXIT_SUCCESS;
}

}// namespace

int main(int argc, char *argv[]) {
    return run(argc, argv);
}
