#include "event_memory.hh"

#include <algorithm>
#include <cstring>
#include <limits>

namespace hojy::event {

EventMemory::EventMemory(): bytes_(ByteCount, 0) {
}

void EventMemory::clear() {
    std::fill(bytes_.begin(), bytes_.end(), std::uint8_t(0));
}

std::optional<std::size_t> EventMemory::byteIndex(std::int32_t address,
                                                  std::size_t byteOffset,
                                                  std::size_t size) const {
    if (address < MinWordAddress || address > MaxWordAddress) {
        return std::nullopt;
    }
    const auto wordIndex = static_cast<std::size_t>(address - MinWordAddress);
    if (wordIndex > WordCount || byteOffset > ByteCount - wordIndex * sizeof(std::int16_t)) {
        return std::nullopt;
    }
    const auto start = wordIndex * sizeof(std::int16_t) + byteOffset;
    if (size > ByteCount - start) {
        return std::nullopt;
    }
    return start;
}

bool EventMemory::readWord(std::int32_t address, std::int16_t &value) const {
    const auto index = byteIndex(address, 0, sizeof(value));
    if (!index) { return false; }
    std::memcpy(&value, bytes_.data() + *index, sizeof(value));
    return true;
}

bool EventMemory::writeWord(std::int32_t address, std::int16_t value) {
    const auto index = byteIndex(address, 0, sizeof(value));
    if (!index) { return false; }
    std::memcpy(bytes_.data() + *index, &value, sizeof(value));
    return true;
}

bool EventMemory::readBytes(std::int32_t address, std::size_t byteOffset,
                            void *destination, std::size_t size) const {
    if (size != 0 && destination == nullptr) { return false; }
    const auto index = byteIndex(address, byteOffset, size);
    if (!index) { return false; }
    if (size != 0) {
        std::memcpy(destination, bytes_.data() + *index, size);
    }
    return true;
}

bool EventMemory::writeBytes(std::int32_t address, std::size_t byteOffset,
                             const void *source, std::size_t size) {
    if (size != 0 && source == nullptr) { return false; }
    const auto index = byteIndex(address, byteOffset, size);
    if (!index) { return false; }
    if (size != 0) {
        std::memcpy(bytes_.data() + *index, source, size);
    }
    return true;
}

bool EventMemory::writeCString(std::int32_t address, std::string_view value) {
    if (value.size() == std::numeric_limits<std::size_t>::max()) { return false; }
    const auto total = value.size() + 1;
    const auto index = byteIndex(address, 0, total);
    if (!index) { return false; }
    std::vector<std::uint8_t> candidate(total, 0);
    if (!value.empty()) {
        std::memcpy(candidate.data(), value.data(), value.size());
    }
    return writeBytes(address, 0, candidate.data(), candidate.size());
}

bool EventMemory::appendCString(std::int32_t address, std::string_view suffix) {
    const auto current = readCString(address);
    if (!current || current->size() > std::numeric_limits<std::size_t>::max() - suffix.size()) {
        return false;
    }
    std::string combined = *current;
    combined.append(suffix.data(), suffix.size());
    return writeCString(address, combined);
}

std::optional<std::string> EventMemory::readCString(std::int32_t address,
                                                    std::size_t maxBytes) const {
    const auto index = byteIndex(address, 0, 0);
    if (!index) { return std::nullopt; }
    const auto available = ByteCount - *index;
    const auto limit = std::min(available, maxBytes);
    const auto *begin = bytes_.data() + *index;
    const auto *end = std::find(begin, begin + limit, std::uint8_t(0));
    if (end == begin + limit) { return std::nullopt; }
    return std::string(reinterpret_cast<const char *>(begin),
                       reinterpret_cast<const char *>(end));
}

}
