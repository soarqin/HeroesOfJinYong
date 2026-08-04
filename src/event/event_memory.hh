#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace hojy::event {

class EventMemory final {
public:
    static constexpr std::int32_t MinWordAddress = -0x8000;
    static constexpr std::int32_t MaxWordAddress = 0x7FFF;
    static constexpr std::size_t WordCount = 0x10000;
    static constexpr std::size_t ByteCount = WordCount * sizeof(std::int16_t);

    EventMemory();

    void clear();
    [[nodiscard]] bool readWord(std::int32_t address, std::int16_t &value) const;
    [[nodiscard]] bool writeWord(std::int32_t address, std::int16_t value);

    [[nodiscard]] bool readBytes(std::int32_t address, std::size_t byteOffset,
                                 void *destination, std::size_t size) const;
    [[nodiscard]] bool writeBytes(std::int32_t address, std::size_t byteOffset,
                                  const void *source, std::size_t size);

    [[nodiscard]] bool writeCString(std::int32_t address, std::string_view value);
    [[nodiscard]] bool appendCString(std::int32_t address, std::string_view suffix);
    [[nodiscard]] std::optional<std::string> readCString(
        std::int32_t address, std::size_t maxBytes = ByteCount) const;

private:
    [[nodiscard]] std::optional<std::size_t> byteIndex(std::int32_t address,
                                                       std::size_t byteOffset,
                                                       std::size_t size) const;

    std::vector<std::uint8_t> bytes_;
};

}
