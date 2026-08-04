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

#include <algorithm>
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

bool checkedFrameSize(size_t outputSize, size_t inputSampleSize, size_t outputSampleSize,
                     size_t &inputSize) {
    if (inputSampleSize == 0 || outputSampleSize == 0) { return false; }
    const auto frameOut = outputSampleSize * 2;
    if (outputSize / frameOut > std::numeric_limits<size_t>::max() / (inputSampleSize * 2)) {
        return false;
    }
    inputSize = outputSize / frameOut * inputSampleSize * 2;
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
        if (!checkedFrameSize(size, inSize, outSize, size)) { return 0; }
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
    if (size > static_cast<size_t>(std::numeric_limits<int>::max())) { return 0; }
    SDL_AudioCVT cvt{};
    const auto built = SDL_BuildAudioCVT(&cvt, Mixer::convertType(typeIn_), 2, int(sampleRateIn_),
                                         Mixer::convertType(typeOut_), 2, int(sampleRateIn_));
    if (built < 0) { return 0; }
    size_t outputSize = 0;
    if (built > 0 && !checkedConversionSize(size, cvt.len_mult, outputSize)) { return 0; }
    const auto smax = std::max(size, outputSize);
    if (cache_.size() < smax) {
        cache_.resize(smax);
    }
    if (needCopy) {
        memcpy(cache_.data(), *data, size);
    }
    if (built == 0) {
        *data = cache_.data();
        return size;
    }
    cvt.len = static_cast<int>(size);
    cvt.buf = cache_.data();
    if (SDL_ConvertAudio(&cvt) < 0 || cvt.len_cvt < 0) { return 0; }
    *data = cache_.data();
    return static_cast<size_t>(cvt.len_cvt);
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
    ok_ = false;
    if (data_.empty() || data_.size() > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return;
    }

    SDL_AudioSpec spec{};
    Uint8 *loadedBuffer = nullptr;
    Uint32 loadedLength = 0;
    auto *source = SDL_RWFromConstMem(data_.data(), static_cast<int>(data_.size()));
    if (!source || !SDL_LoadWAV_RW(source, 1, &spec, &loadedBuffer, &loadedLength)) {
        return;
    }
    if (spec.freq <= 0 || spec.channels == 0 || loadedLength == 0) {
        SDL_FreeWAV(loadedBuffer);
        return;
    }

    auto format = spec.format;
    const bool supportedFormat = format == AUDIO_F32 || format == AUDIO_S32 || format == AUDIO_S16;
    if (spec.channels != 2 || !supportedFormat) {
        const auto targetFormat = supportedFormat ? format : AUDIO_S16;
        SDL_AudioCVT cvt{};
        const auto built = SDL_BuildAudioCVT(&cvt, spec.format, spec.channels, spec.freq,
                                             targetFormat, 2, spec.freq);
        if (built < 0) {
            SDL_FreeWAV(loadedBuffer);
            return;
        }
        if (built == 0) {
            // A zero result means no conversion is needed; that is only valid
            // when the source already has the target channel count and format.
            if (spec.channels != 2 || spec.format != targetFormat) {
                SDL_FreeWAV(loadedBuffer);
                return;
            }
        } else {
            size_t convertedSize = 0;
            if (!checkedConversionSize(loadedLength, cvt.len_mult, convertedSize)
                || convertedSize > static_cast<size_t>(std::numeric_limits<Uint32>::max())) {
                SDL_FreeWAV(loadedBuffer);
                return;
            }
            const auto capacity = std::max<size_t>(loadedLength, convertedSize);
            auto *converted = static_cast<Uint8 *>(SDL_malloc(capacity));
            if (!converted) {
                SDL_FreeWAV(loadedBuffer);
                return;
            }
            memcpy(converted, loadedBuffer, loadedLength);
            cvt.len = static_cast<int>(loadedLength);
            cvt.buf = converted;
            if (SDL_ConvertAudio(&cvt) < 0 || cvt.len_cvt <= 0
                || static_cast<std::uint64_t>(cvt.len_cvt) > std::numeric_limits<Uint32>::max()) {
                SDL_free(converted);
                SDL_FreeWAV(loadedBuffer);
                return;
            }
            SDL_FreeWAV(loadedBuffer);
            loadedBuffer = converted;
            loadedLength = static_cast<Uint32>(cvt.len_cvt);
        }
        format = targetFormat;
    }

    typeIn_ = Mixer::convertDataType(format);
    if (typeIn_ == Mixer::InvalidType) {
        SDL_FreeWAV(loadedBuffer);
        return;
    }
    buffer_ = loadedBuffer;
    length_ = loadedLength;
    sampleRateIn_ = spec.freq;
    ok_ = true;
}

}
