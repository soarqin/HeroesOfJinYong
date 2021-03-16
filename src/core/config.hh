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

#include <string>
#include <vector>

namespace hojy::core {

class Config {
public:
    bool load(const std::string &filename);

    [[nodiscard]] std::string dataFilePathFirst(const std::string &filename) const;
    [[nodiscard]] std::vector<std::string> dataFilePath(const std::string &filename) const;

    [[nodiscard]] std::string musicFilePath(const std::string &filename) const;
    [[nodiscard]] std::string soundFilePath(const std::string &filename) const;
    [[nodiscard]] std::string saveFilePath(const std::string &filename) const;
    [[nodiscard]] const std::vector<std::string> &fonts() const { return fonts_; }

    [[nodiscard]] int windowWidth() const { return windowWidth_; }
    [[nodiscard]] int windowHeight() const { return windowHeight_; }

    [[nodiscard]] bool showPotential() const { return showPotential_; }
    [[nodiscard]] float scale() const { return scale_; }
    [[nodiscard]] float animationSpeed() const { return animationSpeed_; }
    [[nodiscard]] float fadeSpeed() const { return fadeSpeed_; }
    [[nodiscard]] bool noNameInput() const { return noNameInput_; }
    [[nodiscard]] const std::wstring &defaultName() const { return defaultName_; }

private:
    std::vector<std::string> dataPath_, fonts_;
    std::string musicPath_, soundPath_, savePath_;
    int windowWidth_ = 640, windowHeight_ = 480;
    bool showPotential_ = false;
    float scale_ = 2.f;
    float animationSpeed_ = 1.f;
    float fadeSpeed_ = 1.f;
    bool noNameInput_ = false;
    std::wstring defaultName_;
};

extern Config config;

}
