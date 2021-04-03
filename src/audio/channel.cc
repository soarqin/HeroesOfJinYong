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

#include "channel.hh"

#include <util/file.hh>
#include <map>

namespace hojy::audio {

static std::map<std::string, std::vector<std::uint8_t>> dataCache_;
static const std::vector<std::uint8_t> &loadDataFromCacheOrFile(const std::string &filename) {
    auto &data = dataCache_[filename];
    if (!data.empty()) {
        return data;
    }
    if (util::File::getFileContent(filename, data)) {
        return data;
    }
    static std::vector<std::uint8_t> dummy;
    return dummy;
}

Channel::Channel(Mixer *mixer, const std::string &filename): sampleRateOut_(mixer->sampleRate()), typeOut_(mixer->dataType()), data_(loadDataFromCacheOrFile(filename)), ok_(!data_.empty()) {
}

Channel::Channel(Mixer *mixer, const void *data, size_t size): sampleRateOut_(mixer->sampleRate()), typeOut_(mixer->dataType()), ok_(size > 0) {
    data_.resize(size);
    memcpy(data_.data(), data, size);
}

void Channel::load(const std::string &filename) {
    resampler_.reset();
    data_.clear();
    data_ = loadDataFromCacheOrFile(filename);
    ok_ = !data_.empty();
}

size_t Channel::readData(void *data, size_t size) {
    if (resampler_) {
        return resampler_->read(data, size);
    }
    const void *pcmdata;
    auto res = readPCMData(&pcmdata, size, true);
    if (res) {
        memcpy(data, pcmdata, size);
    }
    return res;
}

void Channel::start() {
    if (sampleRateIn_ != sampleRateOut_) {
#if defined(USE_SOXR)
        resampler_ = std::make_unique<Resampler>(2, sampleRateIn_, sampleRateOut_, typeIn_, typeOut_);
        resampler_->setInputCallback([this](const void **data, size_t size)->size_t {
            return readPCMData(data, size, false);
        });
#else
        resampler_ = std::make_unique<Resampler>(2, sampleRateIn_, sampleRateOut_, Mixer::F32, Mixer::F32);
        resampler_->setInputCallback([this](const void **data, size_t size)->size_t {
            return readPCMData(data, size, true);
        });
#endif
    }
}

}
