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

#include <SDL.h>

namespace hojy::audio {

Mixer::Mixer() {
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_InitSubSystem(SDL_INIT_AUDIO);
    }
    SDL_AudioSpec desired = {
        .freq = 48000,
        .format = AUDIO_S16LSB,
        .channels = 2,
        .samples = 2048,
        .callback = callback,
        .userdata = this,
    };
    SDL_AudioSpec obtained;
    audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    sampleRate_ = obtained.freq;
}

Mixer::~Mixer() {
    SDL_CloseAudioDevice(audioDevice_);
}

void Mixer::pause(bool on) const {
    SDL_PauseAudioDevice(audioDevice_, on ? SDL_TRUE : SDL_FALSE);
}

void Mixer::callback(void *userdata, std::uint8_t *stream, int len) {
    auto *mixer = static_cast<Mixer*>(userdata);
}

}
