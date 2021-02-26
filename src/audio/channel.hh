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

#include "mixer.hh"
#include "resampler.hh"

#include <string_view>
#include <vector>
#include <memory>
#include <cstdint>

namespace hojy::audio {

class Channel {
public:
    Channel(Mixer *mixer, std::string_view filename);
    Channel(Mixer *mixer, const void *data, size_t size);
    virtual ~Channel() = default;

    Channel(const Channel&) = delete;
    Channel(Channel&&) noexcept = default;

    inline void setRepeat(bool r) { repeat_ = r; }
    size_t readData(void *data, size_t size);
    void start();

    virtual void reset() {}

protected:
    virtual size_t readPCMData(const void **data, size_t size) { return 0; }

protected:
    std::vector<std::uint8_t> data_;
    std::unique_ptr<Resampler> resampler_;

    std::uint32_t channels_ = 0;
    double sampleRateIn_ = 0.f, sampleRateOut_ = 0.f;
    Mixer::DataType typeIn_ = Mixer::F32, typeOut_ = Mixer::F32;
    bool repeat_ = false;
};

}
