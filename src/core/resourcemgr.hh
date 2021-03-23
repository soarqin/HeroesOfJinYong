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

#pragma once

#include <set>
#include <map>
#include <string>
#include <cctype>

namespace hojy::core {

class ResourceMgr {
    struct StringCaseInsensitiveLess {
        // case-independent (ci) compare_less binary function
        struct CanseInsensitiveCompare {
            bool operator() (const unsigned char &c1, const unsigned char &c2) const {
                return std::tolower(c1) < std::tolower(c2);
            }
        };
        bool operator() (const std::string &s1, const std::string &s2) const {
            return std::lexicographical_compare(s1.begin (), s1.end (),
                                                s2.begin (), s2.end (),
                                                CanseInsensitiveCompare ());
        }
    };
public:
    void init();
    [[nodiscard]] const std::set<std::string> &missingFiles() const { return missingFiles_; }
    [[nodiscard]] const std::set<std::string> &missingFilesOpt() const { return missingFiles_; }
    [[nodiscard]] const std::string &getFilePath(const std::string &file) const;

private:
    std::set<std::string> missingFiles_, missingFilesOpt_;
    std::map<std::string, std::string, StringCaseInsensitiveLess> files_;
};

extern ResourceMgr gResourceMgr;

}
