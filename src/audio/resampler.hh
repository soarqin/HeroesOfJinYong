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
#include "util/fifobuffer.hh"
#include <functional>
#include <cstdint>

namespace hojy::audio {

class Resampler final {
public:
    using InputCallback = std::function<size_t (const void**, size_t)>;
    Resampler(std::uint32_t channels, double sampleRateIn, double sampleRateOut, Mixer::DataType typeIn, Mixer::DataType typeOut);
    ~Resampler();
    void setInputCallback(InputCallback callback);
    size_t read(void *data, size_t size);

private:
    static size_t readCB(void *userdata, void const **data, size_t len);

private:
    void *resampler_ = nullptr;
    InputCallback inputCB_;
    double rate_ = 0.;
    size_t sampleSizeIn_ = 0, sampleSizeOut_ = 0;
#if !defined(USE_SOXR)
    util::FIFOBuffer buffer_;
#endif
};

}
