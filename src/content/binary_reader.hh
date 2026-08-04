#pragma once

#include <cstddef>
#include <cstring>
#include <string>
#include <type_traits>

namespace hojy::content {

// Bounds-checked view over an already loaded binary blob.  A failed read is
// non-consuming and leaves the destination untouched, which makes it suitable
// for candidate-object deserialization and nested format validation.
class BinaryReader final {
public:
    explicit BinaryReader(const std::string &data) noexcept:
        data_(data.data()), size_(data.size()) {
    }

    BinaryReader(const char *data, std::size_t size) noexcept:
        data_(data), size_(size) {
    }

    [[nodiscard]] std::size_t position() const noexcept { return position_; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }
    [[nodiscard]] std::size_t remaining() const noexcept { return size_ - position_; }

    bool readBytes(void *destination, std::size_t count) noexcept;
    bool skip(std::size_t count) noexcept;
    bool readString(std::size_t count, std::string &destination);

    template<typename T>
    bool readPod(T &destination) noexcept {
        static_assert(std::is_trivially_copyable<T>::value,
                      "BinaryReader::readPod requires a trivially copyable type");
        T candidate{};
        if (!readBytes(&candidate, sizeof(candidate))) {
            return false;
        }
        destination = candidate;
        return true;
    }

private:
    const char *data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t position_ = 0;
};

}
