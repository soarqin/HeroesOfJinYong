#include "file.hh"

#include <cstdio>

namespace hojy::util {

std::string File::getFileContent(std::string_view filename) {
    std::string res;
    File f;
    if (f.open(filename)) {
        auto sz = f.size();
        res.resize(sz);
        f.read(res.data(), sz);
    }
    return res;
}

File::~File() {
    fclose(static_cast<FILE*>(handle_));
}

bool File::create(std::string_view filename) {
    FILE *f = fopen(filename.data(), "wb");
    if (!f) {
        return false;
    }
    handle_ = f;
    return true;
}

bool File::open(std::string_view filename, bool readOnly) {
    FILE *f = fopen(filename.data(), readOnly ? "rb" : "r+b");
    if (!f) {
        return false;
    }
    handle_ = f;
    return true;
}

size_t File::read(void *buf, size_t size) {
    return fread(buf, 1, size, static_cast<FILE*>(handle_));
}

size_t File::write(const void *buf, size_t size) {
    return fwrite(buf, 1, size, static_cast<FILE*>(handle_));
}

#ifdef _MSC_VER
#define fseeko64 _fseeki64
#define ftello64 _ftelli64
#endif

std::uint64_t File::size() {
    auto *f = static_cast<FILE*>(handle_);
    auto pos = ftello64(f);
    fseeko64(f, 0, SEEK_END);
    auto res = ftello64(f);
    fseeko64(f, pos, SEEK_SET);
    return res;
}

std::uint64_t File::pos() {
    return ftello64(static_cast<FILE*>(handle_));
}

std::uint64_t File::seek(std::int64_t pos, File::SeekDir type) {
    auto *f = static_cast<FILE*>(handle_);
    fseeko64(f, pos, type);
    return ftello64(f);
}

bool File::eof() {
    return feof(static_cast<FILE*>(handle_));
}

}
