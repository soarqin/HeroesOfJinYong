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

#include "mixer.hh"

#include "channel.hh"

#include <SDL.h>

namespace hojy::audio {

void Mixer::ChannelInfo::reset() {
    ch.reset();
    volume = 0;
}

Mixer::Mixer() {
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_InitSubSystem(SDL_INIT_AUDIO);
    }
    SDL_AudioSpec desired = {
        .freq = 48000,
        .format = AUDIO_S16,
        .channels = 2,
        .samples = 2048,
        .callback = callback,
        .userdata = this,
    };
    SDL_AudioSpec obtained;
    audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    sampleRate_ = obtained.freq;
    format_ = obtained.format;
}

Mixer::~Mixer() {
    SDL_CloseAudioDevice(audioDevice_);
}

void Mixer::play(size_t channelId, Channel *ch, int volume, double fadeIn, double fadeOut) {
    if (channelId >= ChannelMax) {
        return;
    }
    if (channelId >= channels_.size()) {
        channels_.resize(channelId + 1);
    }
    channels_[channelId] = { std::unique_ptr<Channel>(ch), volume };
    channels_.back().ch->start();
}

void Mixer::pause(bool on) const {
    SDL_PauseAudioDevice(audioDevice_, on ? SDL_TRUE : SDL_FALSE);
}

Mixer::DataType Mixer::convertDataType(std::uint16_t type) {
    switch (type) {
    case AUDIO_F32:
        return F32;
    case AUDIO_S16:
        return I16;
    case AUDIO_S32:
        return I32;
    default:
        return InvalidType;
    }
}

void Mixer::callback(void *userdata, std::uint8_t *stream, int len) {
    auto *mixer = static_cast<Mixer*>(userdata);
    memset(stream, 0, len);
    std::vector<std::uint8_t> channelData(len);
    for (auto &chi: mixer->channels_) {
        if (!chi.ch) { continue; }
        auto rsize = chi.ch->readData(channelData.data(), len);
        if (rsize) {
            SDL_MixAudioFormat(stream, channelData.data(), mixer->format_, rsize, chi.volume);
        } else {
            chi.reset();
        }
    }
}

}
