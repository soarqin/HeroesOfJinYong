#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <cstdint>

namespace hojy::util {

class File final {
public:
    enum SeekDir : int {
        Beg = 0,
        Cur = 1,
        End = 2,
    };

public:
    [[nodiscard]] static std::string getFileContent(std::string_view filename);
    template<typename T>
    static bool getFileContent(std::string_view filename, std::vector<T> &data) {
        auto f = File::open(filename);
        if (!f) {
            return false;
        }
        auto sz = f.size() / sizeof(T);
        data.resize(sz);
        f.read(data.data(), sz * sizeof(T));
        return true;
    }

    [[nodiscard]] static File create(std::string_view filename);
    [[nodiscard]] static File open(std::string_view filename, bool readOnly = true);

public:
    File() = default;
    File(const File &) = delete;
    File(File &&other) noexcept: handle_(other.handle_) { other.handle_ = nullptr; }
    ~File();
    File &operator=(const File &) = delete;
    size_t read(void *buf, size_t size);
    size_t write(const void *buf, size_t size);
    std::uint64_t size();
    std::uint64_t pos();
    std::uint64_t seek(std::int64_t pos, SeekDir type = Beg);
    bool eof();
    explicit operator bool() { return handle_ != nullptr; }
    bool operator !() { return handle_ == nullptr;}

private:
    void *handle_ = nullptr;
};

}
