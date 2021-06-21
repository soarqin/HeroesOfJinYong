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
#if defined(USE_SOXR)
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
#else
    format = AUDIO_F32;
#endif
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
    if (!(audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_CHANNELS_CHANGE))) {
        audioDevice_ = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_CHANNELS_CHANGE);
    }
    sampleRate_ = obtained.freq;
    format_ = obtained.format;
    channels_.resize(channels);
    cache_.resize(obtained.size);
}

void Mixer::play(size_t channelId, Channel *ch, int volume, std::uint32_t fadeOutMs, std::uint32_t fadeInMs) {
    if (channelId >= channels_.size()) {
        return;
    }
    std::scoped_lock lk(playMutex_);
    auto &chi = channels_[channelId];
    if (ch) {
        if (!ch->ok()) {
            delete ch;
            return;
        }
        if (fadeOutMs && chi.ch) {
            chi.chNext.reset(ch);
            chi.filenameNext.clear();
            chi.volumeNext = volume;
            auto now = SDL_GetTicks();
            chi.fadeOutStart = now;
            chi.fadeOut = fadeOutMs;
            chi.fadeInStart = chi.fadeIn = 0;
        } else {
            chi = {std::unique_ptr<Channel>(ch), volume};
            ch->start();
        }
        if (fadeInMs) {
            auto now = SDL_GetTicks();
            chi.fadeInStart = now;
            chi.fadeIn = fadeInMs;
        }
    } else {
        if (fadeOutMs && chi.ch) {
            chi.chNext.reset();
            chi.filenameNext.clear();
            auto now = SDL_GetTicks();
            chi.fadeOutStart = now;
            chi.fadeOut = fadeOutMs;
            chi.fadeInStart = chi.fadeIn = 0;
        } else {
            chi = {nullptr, VolumeMax};
        }
    }
}

bool iequals(const std::string &a, const std::string &b) {
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
                      [](unsigned char a, unsigned char b) {
                          return std::toupper(a) == std::toupper(b);
                      });
}

void Mixer::play(size_t channelId, const std::string &filename, bool repeat, int volume, std::uint32_t fadeOutMs, std::uint32_t fadeInMs) {
    if (channelId >= channels_.size()) {
        return;
    }
    std::scoped_lock lk(playMutex_);
    auto &chi = channels_[channelId];
    if (fadeOutMs && chi.ch) {
        chi.chNext.reset();
        chi.filenameNext = filename;
        chi.volumeNext = volume;
        chi.repeatNext = repeat;
        auto now = SDL_GetTicks();
        chi.fadeOutStart = now;
        chi.fadeOut = fadeOutMs;
        chi.fadeInStart = chi.fadeIn = 0;
    } else {
        auto pos = filename.find_last_of('.');
        if (pos == std::string::npos) {
            return;
        }
        auto ext = filename.substr(pos + 1);
        int type = -1;
        if (iequals(ext, "MID") || iequals(ext, "XMI")) {
            type = 0;
            if (!dynamic_cast<ChannelMIDI*>(chi.ch.get())) {
                chi.ch.reset();
            }
        } else if (iequals(ext, "WAV")) {
            type = 1;
            if (!dynamic_cast<ChannelWav*>(chi.ch.get())) {
                chi.ch.reset();
            }
        }
        if (!chi.ch) {
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
            chi = {std::unique_ptr<Channel>(ch), volume};
            chi.ch->start();
        } else {
            chi.ch->load(filename);
            chi.volume = volume;
            chi.ch->start();
        }
        chi.ch->setRepeat(repeat);
    }
    if (fadeInMs) {
        auto now = SDL_GetTicks();
        chi.fadeInStart = now;
        chi.fadeIn = fadeInMs;
    }
}

void Mixer::pause(bool on) const {
    SDL_PauseAudioDevice(audioDevice_, on ? SDL_TRUE : SDL_FALSE);
}

void Mixer::setVolume(size_t channelId, int volume) {
    if (channelId >= channels_.size()) { return; }
    auto &chi = channels_[channelId];
    if (!chi.ch) { return; }
    chi.volume = chi.volumeNext = volume;
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

std::uint16_t Mixer::convertType(Mixer::DataType dtype) {
    switch (dtype) {
    case I16:
        return AUDIO_S16;
    case I32:
        return AUDIO_S32;
    default:
        return AUDIO_F32;
    }
}

size_t Mixer::dataTypeToSize(Mixer::DataType type) {
    switch (type) {
    case Mixer::F32:
        return 4;
    case Mixer::F64:
        return 8;
    case Mixer::I32:
        return 4;
    case Mixer::I16:
        return 2;
    default:
        return 1;
    }
}

void Mixer::callback(void *userdata, std::uint8_t *stream, int len) {
    auto *mixer = static_cast<Mixer*>(userdata);
    std::scoped_lock lk(mixer->playMutex_);
    auto &cache = mixer->cache_;
    auto &channels = mixer->channels_;
    auto &chi0 = channels[0];
    memset(stream, 0, len);
    for (auto &chi: channels) {
        if (!chi.ch) { continue; }
        auto rsize = chi.ch->readData(cache.data(), len);
        if (rsize) {
            if (chi.fadeOut) {
                auto delta = std::uint32_t(std::int32_t(SDL_GetTicks() - chi.fadeOutStart));
                if (delta >= chi.fadeOut) {
                    if (chi.chNext) {
                        chi.ch = std::move(chi.chNext);
                        chi.volume = chi.volumeNext;
                    } else if (!chi.filenameNext.empty()) {
                        auto filename = std::move(chi.filenameNext);
                        auto pos = filename.find_last_of('.');
                        if (pos == std::string::npos) {
                            continue;
                        }
                        auto ext = filename.substr(pos + 1);
                        int type = -1;
                        if (iequals(ext, "MID") || iequals(ext, "XMI")) {
                            type = 0;
                            if (!dynamic_cast<ChannelMIDI*>(chi.ch.get())) {
                                chi.ch.reset();
                            }
                        } else if (iequals(ext, "WAV")) {
                            type = 1;
                            if (!dynamic_cast<ChannelWav*>(chi.ch.get())) {
                                chi.ch.reset();
                            }
                        }
                        if (!chi.ch) {
                            Channel *ch;
                            switch (type) {
                            case 0:
                                ch = new(std::nothrow) ChannelMIDI(mixer, filename);
                                break;
                            case 1:
                                ch = new(std::nothrow) ChannelWav(mixer, filename);
                                break;
                            default:
                                continue;
                            }
                            if (!ch) { continue; }
                            if (!ch->ok()) {
                                delete ch;
                                continue;
                            }
                            chi = {std::unique_ptr<Channel>(ch), chi.volumeNext};
                        } else {
                            chi.ch->load(filename);
                            chi.volume = chi.volumeNext;
                        }
                        chi.ch->start();
                        chi.ch->setRepeat(chi.repeatNext);
                    }
                    chi.fadeOutStart = chi.fadeOut = 0;
                    chi.ch->start();
                } else {
                    int volume = int(chi.volume * (chi.fadeOut - delta) / chi.fadeOut);
                    if (volume) { SDL_MixAudioFormat(stream, cache.data(), mixer->format_, rsize, volume); }
                    continue;
                }
            }
            if (chi.fadeIn) {
                auto delta = std::uint32_t(std::int32_t(SDL_GetTicks() - chi.fadeInStart));
                if (delta >= chi.fadeIn) {
                    chi.fadeInStart = chi.fadeIn = 0;
                } else {
                    int volume = int(chi.volume * delta / chi.fadeIn);
                    if (volume) { SDL_MixAudioFormat(stream, cache.data(), mixer->format_, rsize, volume); }
                    continue;
                }
            }
            if (chi.volume) { SDL_MixAudioFormat(stream, cache.data(), mixer->format_, rsize, chi.volume); }
        } else {
            chi.reset();
        }
    }
}

}
