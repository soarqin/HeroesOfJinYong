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

#if defined(USE_SOXR)
#include <soxr.h>
#else
#include <zita-resampler/vresampler.h>
#endif
#include <cmath>

namespace hojy::audio {

Resampler::Resampler(std::uint32_t channels, double sampleRateIn, double sampleRateOut,
                     Mixer::DataType typeIn, Mixer::DataType typeOut):
                     rate_(sampleRateOut / sampleRateIn) {
#if defined(USE_SOXR)
    auto io_spec = soxr_io_spec(soxr_datatype_t(typeIn), soxr_datatype_t(typeOut));
    auto *resampler = soxr_create(sampleRateIn, sampleRateOut, channels, nullptr, &io_spec, nullptr, nullptr);
#else
    auto *resampler = new VResampler;
    resampler->setup(rate_, channels, 48);
#endif
    resampler_ = resampler;
    sampleSizeIn_ = Mixer::dataTypeToSize(typeIn);
    sampleSizeOut_ = Mixer::dataTypeToSize(typeOut);
    sampleSizeIn_ *= channels;
    sampleSizeOut_ *= channels;
    buffer_.setUnitSize(sampleSizeOut_);
}

void Resampler::setInputCallback(Resampler::InputCallback callback) {
    inputCB_ = std::move(callback);
#if defined(USE_SOXR)
    if (inputCB_) {
        soxr_set_input_fn(static_cast<soxr_t>(resampler_), readCB, this, 0);
    } else {
        soxr_set_input_fn(static_cast<soxr_t>(resampler_), nullptr, this, 0);
    }
#endif
}

size_t Resampler::read(void *data, size_t size) {
    if (inputCB_) {
#if defined(USE_SOXR)
        return soxr_output(static_cast<soxr_t>(resampler_), data, size / sampleSizeOut_) * sampleSizeOut_;
#else
#endif
    } else {
        return buffer_.pop(data, size / sampleSizeOut_, 0);
    }
}

size_t Resampler::write(const void *data, size_t size) {
    if (inputCB_) {
        return 0;
    }
#if defined(USE_SOXR)
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
#else
/*
    auto *resampler = static_cast<VResampler*>(resampler_);
    resampler->inp_data = (float*)data;
    resampler->out_data = (float*)odata;
    resampler->inp_count = isize / resampler->nchan();
    resampler->out_count = osize / resampler->nchan();
    resampler->process();
*/
#endif
}

size_t Resampler::readCB(void *userdata, const void **data, size_t len) {
    auto *resampler = static_cast<Resampler*>(userdata);
    auto sampleSize = resampler->sampleSizeIn_;
    return resampler->inputCB_(data, len * sampleSize) / sampleSize;
}

}
