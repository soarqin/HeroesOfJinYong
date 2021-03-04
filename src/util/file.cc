/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "file.hh"

#include <cstdio>

namespace hojy::util {

File File::create(const std::string &filename) {
    FILE *f = fopen(filename.data(), "wb");
    File file;
    file.handle_ = f;
    return file;
}

File File::open(const std::string &filename, bool readOnly) {
    FILE *f = fopen(filename.data(), readOnly ? "rb" : "r+b");
    File file;
    file.handle_ = f;
    return file;
}

File File::open(const std::vector<std::string> &filename, bool readOnly) {
    File file;
    for (auto &fn: filename) {
        FILE *f = fopen(fn.data(), readOnly ? "rb" : "r+b");
        if (!f) {
            continue;
        }
        file.handle_ = f;
        break;
    }
    return file;
}

File::~File() {
    fclose(static_cast<FILE*>(handle_));
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
