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
    static std::string getFileContent(std::string_view filename);
    template<typename T>
    static bool getFileContent(std::string_view filename, std::vector<T> &data) {
        File f;
        if (!f.open(filename)) {
            return false;
        }
        auto sz = f.size() / sizeof(T);
        data.resize(sz);
        f.read(data.data(), sz * sizeof(T));
        return true;
    }

public:
    ~File();
    bool create(std::string_view filename);
    bool open(std::string_view filename, bool readOnly = true);
    size_t read(void *buf, size_t size);
    size_t write(const void *buf, size_t size);
    std::uint64_t size();
    std::uint64_t pos();
    std::uint64_t seek(std::int64_t pos, SeekDir type = Beg);
    bool eof();

private:
    void *handle_ = nullptr;
};

}
