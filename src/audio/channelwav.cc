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

ChannelWav::ChannelWav(Mixer *mixer, const std::string &filename) : Channel(mixer, filename) {
    if (ok_) { loadFromData(); }
}

ChannelWav::~ChannelWav() {
    if (buffer_) {
        SDL_FreeWAV(buffer_);
    }
}

void ChannelWav::load(const std::string &filename) {
    Channel::load(filename);
    if (!ok_) { return; }
    reset();
    loadFromData();
}

size_t ChannelWav::readPCMData(const void **data, size_t size, bool convType) {
    bool needConv = convType && typeIn_ != typeOut_;
    bool needCopy = false;
    if (needConv) {
        size_t inSize = Mixer::dataTypeToSize(typeIn_);
        size_t outSize = Mixer::dataTypeToSize(typeOut_);
        size = size / outSize / 2 * inSize * 2;
    }
    if (repeat_) {
        if (!length_) { return 0; }
        if (pos_ + size <= length_) {
            *data = buffer_ + pos_;
            pos_ = (pos_ + size) % length_;
            needCopy = true;
        } else {
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
        }
    } else {
        if (pos_ >= length_) { return 0; }
        *data = buffer_ + pos_;
        if (pos_ + size > length_) {
            size = length_ - pos_;
        }
        pos_ += size;
        needCopy = true;
    }
    if (!needConv) {
        return size;
    }
    SDL_AudioCVT cvt;
    SDL_BuildAudioCVT(&cvt, Mixer::convertType(typeIn_), 2, int(sampleRateIn_),
                      Mixer::convertType(typeOut_), 2, int(sampleRateIn_));
    int osize = int(size * cvt.len_mult);
    int smax = std::max<int>(size, osize);
    if (cache_.size() < smax) {
        cache_.resize(smax);
    }
    if (needCopy) {
        memcpy(cache_.data(), *data, size);
    }
    cvt.len = size;
    cvt.buf = cache_.data();
    SDL_ConvertAudio(&cvt);
    *data = cache_.data();
    return cvt.len_cvt;
}

void ChannelWav::loadFromData() {
    SDL_AudioSpec spec;
    if (SDL_LoadWAV_RW(SDL_RWFromConstMem(data_.data(), data_.size()),
                       1, &spec, &buffer_, &length_)) {
        sampleRateIn_ = spec.freq;
        auto format = spec.format;
        if (spec.channels != 2 || (format != AUDIO_F32 && format != AUDIO_S32 && format != AUDIO_S16)) {
            SDL_AudioCVT cvt;
            format = format != AUDIO_F32 && format != AUDIO_S32 && format != AUDIO_S16 ? AUDIO_S16 : format;
            SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq, format, 2, spec.freq);
            cvt.len = length_;
            buffer_ = static_cast<Uint8*>(SDL_realloc(buffer_, length_ * cvt.len_mult));
            cvt.buf = buffer_;
            SDL_ConvertAudio(&cvt);
            length_ = cvt.len_cvt;
        }
        typeIn_ = Mixer::convertDataType(format);
        ok_ = true;
    } else {
        ok_ = false;
    }
}

}
