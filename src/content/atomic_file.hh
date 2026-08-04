#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace hojy::content {

struct AtomicFileEntry {
    std::filesystem::path destination;
    std::string data;
};

class AtomicFile final {
public:
    // Writes to a sibling temporary file and replaces the destination only
    // after the complete payload has been flushed and closed.
    [[nodiscard]] static bool write(const std::filesystem::path &destination,
                                    const std::string &data);

    [[nodiscard]] static bool writePair(const std::vector<AtomicFileEntry> &entries);
};

}
