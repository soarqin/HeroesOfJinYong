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

#include "channelwav.hh"

#include <SDL.h>

namespace hojy::audio {

ChannelWav::ChannelWav(Mixer *mixer, std::string_view filename) : Channel(mixer, filename) {
    load();
}

ChannelWav::ChannelWav(Mixer *mixer, const void *data, size_t size) : Channel(mixer, data, size) {
    load();
}

ChannelWav::~ChannelWav() {
    if (buffer_) {
        SDL_FreeWAV(buffer_);
    }
}

size_t ChannelWav::readPCMData(const void **data, size_t size) {
    if (repeat_) {
        if (!length_) { return 0; }
        if (pos_ + size <= length_) {
            *data = buffer_ + pos_;
            pos_ = (pos_ + size) % length_;
            return size;
        }
        if (cache_.size() < size) {
            cache_.resize(size);
        }
        auto *writedata = cache_.data();
        *data = writedata;
        auto left = size;
        while (left) {
            if (pos_ + left >= length_) {
                auto readsz = length_ - pos_;
                memcpy(writedata, buffer_ + pos_, readsz);
                left -= readsz;
                reset();
            } else {
                memcpy(writedata, buffer_ + pos_, left);
                pos_ += left;
                left = 0;
            }
        }
        return size;
    }
    if (pos_ >= length_) { return 0; }
    *data = buffer_ + pos_;
    if (pos_ + size > length_) {
        size = length_ - pos_;
    }
    pos_ += size;
    return size;
}

void ChannelWav::load() {
    SDL_AudioSpec spec;
    if (SDL_LoadWAV_RW(SDL_RWFromConstMem(data_.data(), data_.size()),
                       1, &spec, &buffer_, &length_)) {
        channels_ = spec.channels;
        sampleRateIn_ = 44100.;
        typeIn_ = Mixer::convertDataType(spec.format);
    }
}

}
