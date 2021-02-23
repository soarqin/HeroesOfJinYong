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
        .callback = nullptr,
        .userdata = nullptr,
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

}
