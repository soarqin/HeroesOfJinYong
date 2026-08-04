#include "atomic_file.hh"

#include <atomic>
#include <fstream>
#include <limits>
#include <set>
#include <system_error>
#include <utility>

#ifdef _WIN32
#include <windows.h>
#endif

namespace hojy::content {
namespace {

std::filesystem::path temporaryPath(const std::filesystem::path &destination) {
    static std::atomic<unsigned long long> sequence{0};
    const auto id = sequence.fetch_add(1, std::memory_order_relaxed);
    return destination.string() + ".tmp." + std::to_string(id);
}

std::filesystem::path backupPath(const std::filesystem::path &destination) {
    static std::atomic<unsigned long long> sequence{0};
    const auto id = sequence.fetch_add(1, std::memory_order_relaxed);
    return destination.string() + ".bak." + std::to_string(id);
}

bool replaceFile(const std::filesystem::path &temporary,
                 const std::filesystem::path &destination) {
#ifdef _WIN32
    return MoveFileExW(temporary.c_str(), destination.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    std::error_code error;
    std::filesystem::rename(temporary, destination, error);
    return !error;
#endif
}

bool movePath(const std::filesystem::path &source,
              const std::filesystem::path &destination) {
#ifdef _WIN32
    return MoveFileExW(source.c_str(), destination.c_str(), MOVEFILE_WRITE_THROUGH) != 0;
#else
    std::error_code error;
    std::filesystem::rename(source, destination, error);
    return !error;
#endif
}

void removePath(const std::filesystem::path &path) {
    std::error_code error;
    std::filesystem::remove(path, error);
}

}

bool AtomicFile::write(const std::filesystem::path &destination,
                       const std::string &data) {
    return writePair({AtomicFileEntry{destination, data}});
}

bool AtomicFile::writePair(const std::vector<AtomicFileEntry> &entries) {
    if (entries.empty()) {
        return false;
    }

    std::set<std::filesystem::path> destinations;
    for (const auto &entry: entries) {
        if (entry.destination.empty()
            || std::filesystem::is_directory(entry.destination)
            || (!entry.destination.parent_path().empty()
                && !std::filesystem::is_directory(entry.destination.parent_path()))
            || entry.data.size() > static_cast<std::size_t>(
                std::numeric_limits<std::streamsize>::max())
            || !destinations.insert(entry.destination).second) {
            return false;
        }
    }

    std::vector<std::filesystem::path> temporaryFiles;
    temporaryFiles.reserve(entries.size());
    for (const auto &entry: entries) {
        const auto temporary = temporaryPath(entry.destination);
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output) {
            for (const auto &path: temporaryFiles) { removePath(path); }
            return false;
        }
        output.write(entry.data.data(), static_cast<std::streamsize>(entry.data.size()));
        output.flush();
        if (!output) {
            output.close();
            removePath(temporary);
            for (const auto &path: temporaryFiles) { removePath(path); }
            return false;
        }
        output.close();
        temporaryFiles.push_back(temporary);
    }

    struct Backup {
        std::filesystem::path destination;
        std::filesystem::path path;
    };
    std::vector<Backup> backups;
    backups.reserve(entries.size());
    for (const auto &entry: entries) {
        if (!std::filesystem::exists(entry.destination)) {
            continue;
        }
        const auto backup = backupPath(entry.destination);
        if (!movePath(entry.destination, backup)) {
            for (const auto &item: backups) {
                movePath(item.path, item.destination);
            }
            for (const auto &path: temporaryFiles) { removePath(path); }
            return false;
        }
        backups.push_back(Backup{entry.destination, backup});
    }

    std::size_t committed = 0;
    for (; committed < entries.size(); ++committed) {
        if (!replaceFile(temporaryFiles[committed], entries[committed].destination)) {
            for (std::size_t i = 0; i < committed; ++i) {
                removePath(entries[i].destination);
            }
            for (const auto &item: backups) {
                movePath(item.path, item.destination);
            }
            for (std::size_t i = committed; i < temporaryFiles.size(); ++i) {
                removePath(temporaryFiles[i]);
            }
            return false;
        }
    }

    for (const auto &item: backups) {
        removePath(item.path);
    }
    return true;
}

}
