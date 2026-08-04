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

#include "channelmidi.hh"

#include "core/config.hh"
#include "sample_bounds.hh"
#include <adlmidi.h>
#include <SDL.h>

#include <algorithm>
#include <limits>

namespace hojy::audio {

ChannelMIDI::ChannelMIDI(Mixer *mixer, const std::string &filename) : Channel(mixer, filename) {
    if (ok_) { loadFromData(); }
}

ChannelMIDI::~ChannelMIDI() {
    if (midiplayer_) {
        adl_close(static_cast<ADL_MIDIPlayer*>(midiplayer_));
        midiplayer_ = nullptr;
    }
}

void ChannelMIDI::load(const std::string &filename) {
    if (midiplayer_) {
        adl_reset(static_cast<ADL_MIDIPlayer*>(midiplayer_));
    }
    Channel::load(filename);
    if (!ok_) { return; }
    loadFromData();
}

void ChannelMIDI::reset() {
    adl_positionRewind(static_cast<ADL_MIDIPlayer*>(midiplayer_));
}

void ChannelMIDI::setRepeat(bool r) {
    Channel::setRepeat(r);
    adl_setLoopEnabled(static_cast<ADL_MIDIPlayer*>(midiplayer_), r ? 1 : 0);
}

size_t ChannelMIDI::readPCMData(const void **data, size_t size, bool convType) {
    bool needConv = convType && typeIn_ != typeOut_;
    int count;
    if (needConv) {
        size_t outSize = Mixer::dataTypeToSize(typeOut_);
        if (outSize == 0 || size / outSize / 2 >
            static_cast<size_t>(std::numeric_limits<int>::max() / 2)) {
            return 0;
        }
        count = static_cast<int>(size / outSize / 2) * 2;
        size = static_cast<size_t>(count) * sizeof(short);
    } else {
        if (size / sizeof(short) > static_cast<size_t>(std::numeric_limits<int>::max())) {
            return 0;
        }
        count = static_cast<int>(size / sizeof(short));
    }
    if (cache_.size() < size) {
        cache_.resize(size);
    }
    auto res = adl_play(static_cast<ADL_MIDIPlayer *>(midiplayer_), count, reinterpret_cast<short *>(cache_.data()));
    if (res < 0) {
        return 0;
    }
    size_t sampleBytes = 0;
    if (!detail::checkedMidiSampleBytes(res, sampleBytes)) { return 0; }
    if (!needConv) {
        *data = cache_.data();
        return sampleBytes;
    }
    SDL_AudioCVT cvt{};
    const auto built = SDL_BuildAudioCVT(&cvt, Mixer::convertType(typeIn_), 2,
                                         int(sampleRateIn_), Mixer::convertType(typeOut_),
                                         2, int(sampleRateIn_));
    if (built < 0) { return 0; }
    if (built == 0) {
        *data = cache_.data();
        return static_cast<size_t>(res * sizeof(short));
    }
    int isize = 0;
    if (!detail::checkedAudioCvtLength(sampleBytes, isize)
        || cvt.len_mult <= 0
        || sampleBytes > std::numeric_limits<size_t>::max()
            / static_cast<size_t>(cvt.len_mult)) {
        return 0;
    }
    const auto osize = sampleBytes * static_cast<size_t>(cvt.len_mult);
    if (cache_.size() < std::max(sampleBytes, osize)) {
        cache_.resize(std::max(sampleBytes, osize));
    }
    cvt.len = isize;
    cvt.buf = cache_.data();
    if (SDL_ConvertAudio(&cvt) < 0 || cvt.len_cvt < 0) { return 0; }
    *data = cache_.data();
    return static_cast<size_t>(cvt.len_cvt);
}

void ChannelMIDI::loadFromData() {
    if (!midiplayer_) {
        midiplayer_ = adl_init(ADL_CHIP_SAMPLE_RATE);
        if (!midiplayer_) {
            ok_ = false;
            return;
        }
        const auto &emu = core::config.oplEmulator();
        if (emu == "nuked174")
            adl_switchEmulator(static_cast<ADL_MIDIPlayer *>(midiplayer_), ADLMIDI_EMU_NUKED_174);
        else if (emu == "nuked")
            adl_switchEmulator(static_cast<ADL_MIDIPlayer *>(midiplayer_), ADLMIDI_EMU_NUKED);
        else
            adl_switchEmulator(static_cast<ADL_MIDIPlayer *>(midiplayer_), ADLMIDI_EMU_DOSBOX);
    }
    unsigned long dataSize = 0;
    if (!detail::checkedAdlDataSize(data_.size(), dataSize)
        || adl_openData(static_cast<ADL_MIDIPlayer*>(midiplayer_),
                        data_.data(), dataSize) < 0) {
        ok_ = false;
        return;
    }
    sampleRateIn_ = ADL_CHIP_SAMPLE_RATE;
    typeIn_ = Mixer::I16;
    ok_ = true;
}

}
