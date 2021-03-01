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

#include <adlmidi.h>

namespace hojy::audio {

ChannelMIDI::ChannelMIDI(Mixer *mixer, std::string_view filename) : Channel(mixer, filename) {
    load();
}

ChannelMIDI::ChannelMIDI(Mixer *mixer, const void *data, size_t size) : Channel(mixer, data, size) {
    load();
}

ChannelMIDI::~ChannelMIDI() {
    if (midiplayer_) {
        adl_close(static_cast<ADL_MIDIPlayer*>(midiplayer_));
        midiplayer_ = nullptr;
    }
}

void ChannelMIDI::reset() {
    adl_positionRewind(static_cast<ADL_MIDIPlayer*>(midiplayer_));
}

size_t ChannelMIDI::readPCMData(const void **data, size_t size) {
    auto count = size / sizeof(short);
    if (cache_.size() < count) {
        cache_.resize(count);
    }
    auto res = adl_play(static_cast<ADL_MIDIPlayer*>(midiplayer_), count, cache_.data());
    if (res < 0) {
        return 0;
    }
    if (res < size && adl_atEnd(static_cast<ADL_MIDIPlayer*>(midiplayer_)) && repeat_) {
        reset();
        auto res2 = adl_play(static_cast<ADL_MIDIPlayer*>(midiplayer_), count - res / sizeof(short), cache_.data() + res / sizeof(short));
        if (res2 < 0) {
            return res;
        }
        res += res2;
    }
    *data = cache_.data();
    return res * sizeof(short);
}

void ChannelMIDI::load() {
    midiplayer_ = adl_init(ADL_CHIP_SAMPLE_RATE);
    if (!midiplayer_) {
        return;
    }
    // adl_setLoopEnabled(static_cast<ADL_MIDIPlayer*>(midiplayer_), 1);
    if (adl_openData(static_cast<ADL_MIDIPlayer*>(midiplayer_), data_.data(), data_.size()) < 0) {
        return;
    }
    channels_ = 2;
    sampleRateIn_ = ADL_CHIP_SAMPLE_RATE;
    typeIn_ = Mixer::I16;
}

}
