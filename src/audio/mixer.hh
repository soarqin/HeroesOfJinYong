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

#include <vector>
#include <memory>
#include <cstdint>

namespace hojy::audio {

class Channel;

class Mixer final {
public:
    enum DataType {
        InvalidType = -1, F32 = 0,  F64,  I32,  I16,
    };

    Mixer();
    ~Mixer();

    void addChannel(std::unique_ptr<Channel> &&ch);
    void pause(bool on) const;
    [[nodiscard]] inline std::uint32_t sampleRate() const { return sampleRate_; }
    [[nodiscard]] inline DataType dataType() const { return dataType_; }

    static DataType convertDataType(std::uint16_t type);

private:
    static void callback(void *userdata, std::uint8_t * stream, int len);

private:
    std::uint32_t audioDevice_ = 0;
    std::uint32_t sampleRate_ = 0;
    DataType dataType_ = F32;
    std::vector<std::unique_ptr<Channel>> channels_;
};

}
