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

#include "resampler.hh"

#include <soxr.h>
#include <cmath>

namespace hojy::audio {

void DataTypeToSize(Mixer::DataType type, size_t &size) {
    switch (type) {
    case Mixer::F32:
        size = 4;
        break;
    case Mixer::F64:
        size = 8;
        break;
    case Mixer::I32:
        size = 4;
        break;
    case Mixer::I16:
        size = 2;
        break;
    }
}

Resampler::Resampler(std::uint32_t channels, double sampleRateIn, double sampleRateOut, Mixer::DataType typeIn, Mixer::DataType typeOut) {
    auto io_spec = soxr_io_spec(soxr_datatype_t(typeIn), soxr_datatype_t(typeOut));
    auto *resampler = soxr_create(sampleRateIn, sampleRateOut, channels, nullptr, &io_spec, nullptr, nullptr);
    resampler_ = resampler;
    rate_ = sampleRateOut / sampleRateIn;
    DataTypeToSize(typeIn, sampleSizeIn_);
    DataTypeToSize(typeOut, sampleSizeOut_);
    buffer_.setUnitSize(sampleSizeOut_);
}

void Resampler::setInputCallback(Resampler::InputCallback callback) {
    inputCB_ = std::move(callback);
    if (inputCB_) {
        soxr_set_input_fn(static_cast<soxr_t>(resampler_), readCB, this, 65536);
    } else {
        soxr_set_input_fn(static_cast<soxr_t>(resampler_), nullptr, this, 65536);
    }
}

size_t Resampler::read(void *data, size_t size) {
    if (inputCB_) {
        return soxr_output(static_cast<soxr_t>(resampler_), data, size / sampleSizeOut_) * sampleSizeOut_;
    }
    return 0;
}

size_t Resampler::write(const void *data, size_t size) {
    if (inputCB_) {
        return 0;
    }
    size_t idone, odone;
    auto isize = size_t(size / sampleSizeIn_);
    auto osize = size_t(std::ceil(isize * rate_));
    auto *odata = buffer_.alloc(osize);
    if (!odata) {
        return 0;
    }
    if (soxr_process(static_cast<soxr_t>(resampler_), data, isize, &idone, odata, osize, &odone) != nullptr) {
        return 0;
    }
    buffer_.push(odata, odone);
    return idone;
}

size_t Resampler::readCB(void *userdata, const void **data, size_t len) {
    return static_cast<Resampler*>(userdata)->inputCB_(data, len);
}

}
