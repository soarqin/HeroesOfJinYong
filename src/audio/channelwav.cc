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

#include <limits>

namespace hojy::audio {

namespace {

bool checkedConversionSize(size_t inputSize, int multiplier, size_t &outputSize) {
    if (multiplier <= 0
        || inputSize > static_cast<size_t>(std::numeric_limits<int>::max()) / static_cast<size_t>(multiplier)) {
        return false;
    }
    outputSize = inputSize * static_cast<size_t>(multiplier);
    return true;
}

}

ChannelWav::ChannelWav(Mixer *mixer, const std::string &filename) : Channel(mixer, filename) {
    if (ok_) { loadFromData(); }
}

ChannelWav::~ChannelWav() {
    clearBuffer();
}

void ChannelWav::load(const std::string &filename) {
    clearBuffer();
    Channel::load(filename);
    if (!ok_) { return; }
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
        if (pos_ >= length_) { reset(); }
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
                    writedata += readsz;
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
    if (size > std::numeric_limits<int>::max()) { return 0; }
    SDL_AudioCVT cvt{};
    if (SDL_BuildAudioCVT(&cvt, Mixer::convertType(typeIn_), 2, int(sampleRateIn_),
                          Mixer::convertType(typeOut_), 2, int(sampleRateIn_)) < 0) {
        return 0;
    }
    size_t outputSize = 0;
    if (!checkedConversionSize(size, cvt.len_mult, outputSize)) { return 0; }
    auto smax = std::max(size, outputSize);
    if (cache_.size() < smax) {
        cache_.resize(smax);
    }
    if (needCopy) {
        memcpy(cache_.data(), *data, size);
    }
    cvt.len = static_cast<int>(size);
    cvt.buf = cache_.data();
    if (SDL_ConvertAudio(&cvt) < 0) { return 0; }
    *data = cache_.data();
    return cvt.len_cvt;
}

void ChannelWav::clearBuffer() {
    if (buffer_) {
        SDL_FreeWAV(buffer_);
        buffer_ = nullptr;
    }
    length_ = 0;
    pos_ = 0;
}

void ChannelWav::loadFromData() {
    clearBuffer();
    if (data_.size() > std::numeric_limits<int>::max()) {
        ok_ = false;
        return;
    }
    SDL_AudioSpec spec{};
    auto *source = SDL_RWFromConstMem(data_.data(), static_cast<int>(data_.size()));
    if (!source || !SDL_LoadWAV_RW(source, 1, &spec, &buffer_, &length_)) {
        ok_ = false;
        return;
    }
    sampleRateIn_ = spec.freq;
    auto format = spec.format;
    if (spec.channels != 2 || (format != AUDIO_F32 && format != AUDIO_S32 && format != AUDIO_S16)) {
        SDL_AudioCVT cvt{};
        format = format != AUDIO_F32 && format != AUDIO_S32 && format != AUDIO_S16 ? AUDIO_S16 : format;
        if (SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq, format, 2, spec.freq) < 0) {
            clearBuffer();
            ok_ = false;
            return;
        }
        size_t convertedSize = 0;
        if (!checkedConversionSize(length_, cvt.len_mult, convertedSize)) {
            clearBuffer();
            ok_ = false;
            return;
        }
        auto *converted = static_cast<Uint8*>(SDL_realloc(buffer_, convertedSize));
        if (!converted) {
            clearBuffer();
            ok_ = false;
            return;
        }
        buffer_ = converted;
        cvt.len = static_cast<int>(length_);
        cvt.buf = buffer_;
        if (SDL_ConvertAudio(&cvt) < 0) {
            clearBuffer();
            ok_ = false;
            return;
        }
        length_ = static_cast<std::uint32_t>(cvt.len_cvt);
    }
    typeIn_ = Mixer::convertDataType(format);
    ok_ = typeIn_ != Mixer::InvalidType;
    if (!ok_) { clearBuffer(); }
}

}
