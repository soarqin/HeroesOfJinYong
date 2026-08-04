#include "binary_reader.hh"

#include <new>
#include <utility>

namespace hojy::content {

bool BinaryReader::readBytes(void *destination, std::size_t count) noexcept {
    if (count > remaining() || (count > 0 && destination == nullptr)) {
        return false;
    }
    if (count > 0) {
        std::memcpy(destination, data_ + position_, count);
    }
    position_ += count;
    return true;
}

bool BinaryReader::skip(std::size_t count) noexcept {
    if (count > remaining()) {
        return false;
    }
    position_ += count;
    return true;
}

bool BinaryReader::readString(std::size_t count, std::string &destination) {
    if (count > remaining()) {
        return false;
    }
    try {
        std::string candidate;
        if (count > 0) {
            candidate.assign(data_ + position_, count);
        }
        destination = std::move(candidate);
        position_ += count;
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

}
