#include "content/atomic_file.hh"
#include "content/binary_reader.hh"
#include "test_support.hh"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <iostream>
#include <string>

namespace {

class ScopedTempDirectory {
public:
    ScopedTempDirectory(): oldPath_(std::filesystem::current_path()) {
        const auto suffix = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        path_ = std::filesystem::temp_directory_path() / ("hojy-binary-" + std::to_string(suffix));
        std::filesystem::create_directories(path_);
        std::filesystem::current_path(path_);
    }

    ~ScopedTempDirectory() {
        std::error_code ec;
        std::filesystem::current_path(oldPath_, ec);
        std::filesystem::remove_all(path_, ec);
    }

private:
    std::filesystem::path oldPath_;
    std::filesystem::path path_;
};

void readerRejectsShortReadsWithoutAdvancing() {
    hojy::content::BinaryReader reader("abc");
    std::array<char, 4> output{'x', 'x', 'x', 'x'};
    HOJY_CHECK_EQ(reader.readBytes(output.data(), output.size()), false);
    HOJY_CHECK_EQ(reader.position(), 0U);
    HOJY_CHECK_EQ(output[0], 'x');

    std::string text = "unchanged";
    HOJY_CHECK_EQ(reader.readString(4, text), false);
    HOJY_CHECK_EQ(reader.position(), 0U);
    HOJY_CHECK_EQ(text, "unchanged");
}

void readerReadsPodAndBoundedString() {
    const std::uint16_t number = 0x1234;
    std::string bytes(reinterpret_cast<const char *>(&number), sizeof(number));
    bytes += "ok";
    hojy::content::BinaryReader reader(bytes);
    std::uint16_t decoded = 0;
    std::string text;
    HOJY_CHECK_EQ(reader.readPod(decoded), true);
    HOJY_CHECK_EQ(decoded, number);
    HOJY_CHECK_EQ(reader.readString(2, text), true);
    HOJY_CHECK_EQ(text, "ok");
    HOJY_CHECK_EQ(reader.remaining(), 0U);
}

void atomicWriterReplacesOnlyAfterCompleteWrite() {
    ScopedTempDirectory tempDirectory;
    const std::filesystem::path target = "state.bin";
    HOJY_CHECK_EQ(hojy::content::AtomicFile::write(target, "old"), true);
    HOJY_CHECK_EQ(hojy::content::AtomicFile::write(target, "new"), true);

    std::ifstream input(target, std::ios::binary);
    const std::string value((std::istreambuf_iterator<char>(input)), {});
    HOJY_CHECK_EQ(value, "new");
    HOJY_CHECK_EQ(hojy::content::AtomicFile::write("missing/state.bin", "ignored"), false);
    HOJY_CHECK_EQ(std::filesystem::exists("missing/state.bin"), false);
}

}

int main() {
    try {
        readerRejectsShortReadsWithoutAdvancing();
        readerReadsPodAndBoundedString();
        atomicWriterReplacesOnlyAfterCompleteWrite();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
