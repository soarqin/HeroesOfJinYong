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
#include "channelmidi.hh"
#include "channelwav.hh"
#include "core/config.hh"
#include <SDL.h>

namespace hojy::audio {

Mixer gMixer;

void Mixer::ChannelInfo::reset() {
    ch.reset();
    volume = 0;
}

Mixer::~Mixer() {
    SDL_CloseAudioDevice(audioDevice_);
}

void Mixer::init(int channels) {
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_InitSubSystem(SDL_INIT_AUDIO);
    }
    SDL_AudioFormat format;
    switch (core::config.sampleFormat()) {
    case 1:
        format = AUDIO_S32;
        break;
    case 2:
        format = AUDIO_F32;
        break;
    default:
        format = AUDIO_S16;
        break;
    }
    SDL_AudioSpec desired = {
        core::config.sampleRate(),
        format,
        2,
        0,
        2048,
        0,
        0,
        callback,
        this,
    };
    SDL_AudioSpec obtained;
    if (!(audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_SAMPLES_CHANGE))) {
        audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE);
    }
    sampleRate_ = obtained.freq;
    format_ = obtained.format;
    channels_.resize(channels);
    cache_.resize(obtained.size);
}

void Mixer::play(size_t channelId, Channel *ch, int volume, double fadeIn, double fadeOut) {
    /* TODO: implement fade in/out */
    (void)fadeIn; (void)fadeOut;
    if (channelId >= channels_.size()) {
        return;
    }
    std::scoped_lock lk(playMutex_);
    if (ch) {
        if (!ch->ok()) {
            delete ch;
            return;
        }
        channels_[channelId] = {std::unique_ptr<Channel>(ch), volume};
        channels_[channelId].ch->start();
    } else {
        channels_[channelId] = {nullptr, VolumeMax};
    }
}

bool iequals(const std::string &a, const std::string &b) {
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
                      [](unsigned char a, unsigned char b) {
                          return std::toupper(a) == std::toupper(b);
                      });
}

void Mixer::play(size_t channelId, const std::string &filename, bool repeat, int volume, double fadeIn, double fadeOut) {
    /* TODO: implement fade in/out */
    (void)fadeIn; (void)fadeOut;
    if (channelId >= channels_.size()) {
        return;
    }
    std::scoped_lock lk(playMutex_);
    auto pos = filename.find_last_of('.');
    if (pos == std::string::npos) {
        return;
    }
    auto ext = filename.substr(pos + 1);
    int type = -1;
    if (iequals(ext, "MID") || iequals(ext, "XMI")) {
        type = 0;
        if (!dynamic_cast<ChannelMIDI*>(channels_[channelId].ch.get())) {
            channels_[channelId].ch.reset();
        }
    } else if (iequals(ext, "WAV")) {
        type = 1;
        if (!dynamic_cast<ChannelWav*>(channels_[channelId].ch.get())) {
            channels_[channelId].ch.reset();
        }
    }
    if (!channels_[channelId].ch) {
        Channel *ch;
        switch (type) {
        case 0:
            ch = new(std::nothrow) ChannelMIDI(this, filename);
            break;
        case 1:
            ch = new(std::nothrow) ChannelWav(this, filename);
            break;
        default:
            return;
        }
        if (!ch) { return; }
        if (!ch->ok()) {
            delete ch;
            return;
        }
        channels_[channelId] = {std::unique_ptr<Channel>(ch), volume};
    } else {
        channels_[channelId].ch->load(filename);
        channels_[channelId].volume = volume;
    }
    channels_[channelId].ch->setRepeat(repeat);
    channels_[channelId].ch->start();
}

void Mixer::repeatPlay(size_t channelId, Channel *ch, int volume, double fadeIn, double fadeOut) {
    if (ch) {
        ch->setRepeat(true);
    }
    play(channelId, ch, volume, fadeIn, fadeOut);
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
    auto &cache = mixer->cache_;
    std::scoped_lock lk(mixer->playMutex_);
    auto &channels = mixer->channels_;
    auto &chi0 = channels[0];
    if (chi0.volume < VolumeMax) {
        memset(stream, 0, len);
        for (auto &chi: channels) {
            if (!chi.ch) { continue; }
            auto rsize = chi.ch->readData(cache.data(), len);
            if (rsize) {
                SDL_MixAudioFormat(stream, cache.data(), mixer->format_, rsize, chi.volume);
            } else {
                chi.reset();
            }
        }
    } else {
        auto rsize = chi0.ch->readData(stream, len);
        if (!rsize) {
            chi0.reset();
        }
        if (rsize < len) {
            memset(stream + rsize, 0, len - rsize);
        }
        size_t sz = channels.size();
        for (size_t i = 1; i < sz; ++i) {
            auto &chi = channels[i];
            if (!chi.ch) { continue; }
            rsize = chi.ch->readData(cache.data(), len);
            if (rsize) {
                SDL_MixAudioFormat(stream, cache.data(), mixer->format_, rsize, chi.volume);
            } else {
                chi.reset();
            }
        }
    }
}

}
