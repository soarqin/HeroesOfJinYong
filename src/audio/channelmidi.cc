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

ChannelMIDI::ChannelMIDI(Mixer *mixer, const std::string &filename) : Channel(mixer, filename) {
    if (ok_) { loadFromData(); }
}

ChannelMIDI::ChannelMIDI(Mixer *mixer, const void *data, size_t size) : Channel(mixer, data, size) {
    if (ok_) { loadFromData(); }
}

ChannelMIDI::~ChannelMIDI() {
    if (midiplayer_) {
        adl_close(static_cast<ADL_MIDIPlayer*>(midiplayer_));
        midiplayer_ = nullptr;
    }
}

void ChannelMIDI::load(const std::string &filename) {
    Channel::load(filename);
    if (!ok_) { return; }
    adl_reset(static_cast<ADL_MIDIPlayer*>(midiplayer_));
    loadFromData();
}

void ChannelMIDI::reset() {
    adl_positionRewind(static_cast<ADL_MIDIPlayer*>(midiplayer_));
}

void ChannelMIDI::setRepeat(bool r) {
    Channel::setRepeat(r);
    adl_setLoopEnabled(static_cast<ADL_MIDIPlayer*>(midiplayer_), r ? 1 : 0);
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
    *data = cache_.data();
    return res * sizeof(short);
}

void ChannelMIDI::loadFromData() {
    if (!midiplayer_) {
        midiplayer_ = adl_init(ADL_CHIP_SAMPLE_RATE);
        if (!midiplayer_) {
            ok_ = false;
            return;
        }
        adl_switchEmulator(static_cast<ADL_MIDIPlayer *>(midiplayer_), ADLMIDI_EMU_DOSBOX);
    }
    if (adl_openData(static_cast<ADL_MIDIPlayer*>(midiplayer_), data_.data(), data_.size()) < 0) {
        ok_ = false;
        return;
    }
    channels_ = 2;
    sampleRateIn_ = ADL_CHIP_SAMPLE_RATE;
    typeIn_ = Mixer::I16;
    ok_ = true;
}

}
