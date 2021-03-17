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

#include "util/file.hh"

#include <string>

using namespace hojy;

bool loadGrp(const std::string &idx, const std::string &grp, std::vector<std::string> &dset) {
    util::File ifs, ifs2;
    ifs = util::File::open(idx);
    ifs2 = util::File::open(grp);
    if (!ifs || !ifs2) {
        return false;
    }
    size_t count = ifs.size() / sizeof(std::uint32_t);
    size_t fileSize = ifs2.size();
    dset.resize(count);
    std::uint32_t offset = 0;
    for (size_t i = 0; i < count; ++i) {
        std::uint32_t endoffset;
        ifs.read(&endoffset, sizeof(endoffset));
        if (endoffset == 0) {
            endoffset = fileSize;
        }
        if (endoffset > offset) {
            dset[i].resize(endoffset - offset);
            ifs2.seek(offset);
            ifs2.read(dset[i].data(), endoffset - offset);
            offset = endoffset;
        }
    }
    return true;
}

bool saveGrp(const std::string &idx, const std::string &grp, const std::vector<std::string> &dset) {
    util::File ifs, ifs2;
    ifs = util::File::create(idx);
    ifs2 = util::File::create(grp);
    if (!ifs || !ifs2) {
        return false;
    }
    std::uint32_t offset = 0;
    for (auto &d: dset) {
        offset += d.size();
        ifs2.write(d.data(), d.size());
        ifs.write(&offset, sizeof(std::uint32_t));
    }
    return true;
}

int main(int argc, char *argv[]) {
    if (argc < 3) { return -1; }
    std::vector<std::string> merged;
    for (int i = 0; i < 1000; ++i) {
        char idx[256], grp[256];
        sprintf(idx, "%s%03d", argv[1], i);
        sprintf(grp, "%s%03d", argv[2], i);
        std::vector<std::string> single;
        if (!loadGrp(idx, grp, single)) { continue; }
        fprintf(stdout, "loaded %s %s\n", idx, grp);
        if (single.size() > merged.size()) {
            merged.resize(single.size());
        }
        for (size_t j = 0; j < single.size(); ++j) {
            if (single[j].empty()) { continue; }
            if (merged[j].empty()) { merged[j] = single[j]; continue; }
            if (merged[j].size() != single[j].size()) {
                fprintf(stderr, "size mismatch: %d %zd %zu != %zu\n", i, j, merged[j].size(), single[j].size());
            }
        }
    }
    saveGrp(argv[1], argv[2], merged);
    return 0;
}
