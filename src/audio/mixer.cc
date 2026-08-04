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

#include <algorithm>
#include <cctype>
#include <cstring>
#include <limits>
#include <new>

namespace hojy::audio {

Mixer gMixer;

void Mixer::ChannelInfo::reset() {
    ch.reset();
    volume = 0;
    fadeInStart = fadeIn = 0;
    fadeOutStart = fadeOut = 0;
    fadeOutVolumeStart = 0;
    chNext.reset();
    volumeNext = 0;
    fadeInNext = 0;
    repeatNext = false;
    ended = false;
    sourceEnded = false;
    readyPos = 0;
    readySize = 0;
}

Mixer::~Mixer() {
    if (audioDevice_ != 0) {
        SDL_CloseAudioDevice(audioDevice_);
    }
}

bool Mixer::init(int channels) {
    if (channels <= 0) { return false; }
    if (!SDL_WasInit(SDL_INIT_AUDIO) && SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        SDL_Log("Unable to initialize audio: %s", SDL_GetError());
        return false;
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
    SDL_AudioSpec desired{};
    desired.freq = core::config.sampleRate();
    desired.format = format;
    desired.channels = 2;
    desired.samples = 2048;
    desired.callback = callback;
    desired.userdata = this;
    SDL_AudioSpec obtained{};
    auto newDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (newDevice == 0) {
        newDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained,
                                        SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    }
    if (newDevice == 0) {
        SDL_Log("Unable to open audio device: %s", SDL_GetError());
        return false;
    }
    if (obtained.channels != 2 || convertDataType(obtained.format) == InvalidType
        || obtained.freq <= 0 || obtained.size == 0) {
        SDL_Log("Unsupported audio format returned by device");
        SDL_CloseAudioDevice(newDevice);
        return false;
    }
    std::vector<ChannelInfo> newChannels;
    std::vector<std::uint8_t> newCache;
    try {
        newChannels.resize(static_cast<std::size_t>(channels));
        newCache.assign(static_cast<std::size_t>(obtained.size), 0);
    } catch (const std::bad_alloc &) {
        SDL_CloseAudioDevice(newDevice);
        return false;
    }
    {
        std::scoped_lock lk(playMutex_);
        if (audioDevice_ != 0) {
            SDL_PauseAudioDevice(audioDevice_, SDL_TRUE);
            SDL_CloseAudioDevice(audioDevice_);
        }
        audioDevice_ = newDevice;
        sampleRate_ = obtained.freq;
        format_ = obtained.format;
        channels_.swap(newChannels);
        cache_.swap(newCache);
    }
    return true;
}

void Mixer::play(size_t channelId, Channel *ch, int volume, std::uint32_t fadeOutMs, std::uint32_t fadeInMs) {
    std::scoped_lock lk(playMutex_);
    if (channelId >= channels_.size()) {
        delete ch;
        return;
    }
    auto &chi = channels_[channelId];
    if (ch) {
        if (!ch->ok()) {
            delete ch;
            return;
        }
        if (fadeOutMs && chi.ch) {
            chi.chNext.reset(ch);
            chi.volumeNext = volume;
            chi.fadeInNext = fadeInMs;
            chi.fadeOutVolumeStart = chi.volume;
            auto now = SDL_GetTicks();
            chi.fadeOutStart = now;
            chi.fadeOut = fadeOutMs;
            chi.fadeInStart = chi.fadeIn = 0;
        } else {
            chi.reset();
            chi.ch.reset(ch);
            chi.volume = fadeInMs ? 0 : volume;
            chi.volumeNext = volume;
            ch->start();
            prepareChannelLocked(chi);
            fillChannelLocked(chi);
            if (fadeInMs) {
                const auto now = SDL_GetTicks();
                chi.fadeInStart = now;
                chi.fadeIn = fadeInMs;
            }
        }
    } else {
        if (fadeOutMs && chi.ch) {
            chi.chNext.reset();
            chi.fadeInNext = 0;
            chi.fadeOutVolumeStart = chi.volume;
            auto now = SDL_GetTicks();
            chi.fadeOutStart = now;
            chi.fadeOut = fadeOutMs;
            chi.fadeInStart = chi.fadeIn = 0;
        } else {
            chi.reset();
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
    std::scoped_lock lk(playMutex_);
    if (channelId >= channels_.size()) {
        return;
    }
    auto &chi = channels_[channelId];
    if (fadeOutMs && chi.ch) {
        std::unique_ptr<Channel> candidate = createChannelLocked(filename);
        if (!candidate) {
            return;
        }
        candidate->start();
        candidate->setRepeat(repeat);
        chi.chNext = std::move(candidate);
        chi.volumeNext = volume;
        chi.repeatNext = repeat;
        chi.fadeInNext = fadeInMs;
        chi.fadeOutVolumeStart = chi.volume;
        auto now = SDL_GetTicks();
        chi.fadeOutStart = now;
        chi.fadeOut = fadeOutMs;
        chi.fadeInStart = chi.fadeIn = 0;
    } else {
        if (!loadFilenameLocked(chi, filename, repeat, volume)) { return; }
        if (fadeInMs) {
            const auto now = SDL_GetTicks();
            chi.volume = 0;
            chi.fadeInStart = now;
            chi.fadeIn = fadeInMs;
        }
    }
}

void Mixer::pause(bool on) const {
    std::scoped_lock lk(playMutex_);
    if (audioDevice_ != 0) {
        SDL_PauseAudioDevice(audioDevice_, on ? SDL_TRUE : SDL_FALSE);
    }
}

std::unique_ptr<Channel> Mixer::createChannelLocked(const std::string &filename) {
    const auto pos = filename.find_last_of('.');
    if (pos == std::string::npos) {
        return nullptr;
    }
    const auto ext = filename.substr(pos + 1);
    std::unique_ptr<Channel> channel;
    if (iequals(ext, "MID") || iequals(ext, "XMI")) {
        channel = std::make_unique<ChannelMIDI>(this, filename);
    } else if (iequals(ext, "WAV")) {
        channel = std::make_unique<ChannelWav>(this, filename);
    }
    if (!channel || !channel->ok()) {
        return nullptr;
    }
    return channel;
}

bool Mixer::loadFilenameLocked(ChannelInfo &chi, const std::string &filename,
                                bool repeat, int volume) {
    std::unique_ptr<Channel> candidate = createChannelLocked(filename);
    if (!candidate) { return false; }
    candidate->start();
    candidate->setRepeat(repeat);
    chi.reset();
    chi.ch = std::move(candidate);
    chi.volume = volume;
    chi.volumeNext = volume;
    chi.ended = false;
    prepareChannelLocked(chi);
    fillChannelLocked(chi);
    return true;
}

void Mixer::prepareChannelLocked(ChannelInfo &channel) {
    if (!channel.ch) {
        return;
    }
    const auto chunk = std::max<std::size_t>(cache_.size(), 4096);
    const auto capacity = chunk > std::numeric_limits<std::size_t>::max() / 4
        ? std::numeric_limits<std::size_t>::max()
        : chunk * 4;
    if (capacity == std::numeric_limits<std::size_t>::max()) {
        return;
    }
    if (channel.ready.size() < capacity) {
        channel.ready.resize(capacity);
    }
}

void Mixer::fillChannelLocked(ChannelInfo &channel) {
    if (!channel.ch || channel.sourceEnded || channel.ready.empty()) {
        return;
    }
    if (channel.readyPos > 0
        && (channel.readySize == 0
            || channel.readyPos + channel.readySize == channel.ready.size())) {
        if (channel.readySize > 0) {
            std::memmove(channel.ready.data(), channel.ready.data() + channel.readyPos,
                         channel.readySize);
        }
        channel.readyPos = 0;
    }
    const auto freeSize = channel.ready.size() - channel.readyPos - channel.readySize;
    if (freeSize == 0) {
        return;
    }
    auto *destination = channel.ready.data() + channel.readyPos + channel.readySize;
    const auto readSize = std::max<std::size_t>(cache_.size(), 4096);
    const auto request = std::min(freeSize, readSize);
    const auto received = channel.ch->readData(destination, request);
    if (received == 0) {
        channel.sourceEnded = true;
        if (channel.readySize == 0) {
            channel.ended = true;
        }
        return;
    }
    channel.readySize += std::min(received, request);
}

void Mixer::setVolume(size_t channelId, int volume) {
    std::scoped_lock lk(playMutex_);
    if (channelId >= channels_.size()) { return; }
    auto &chi = channels_[channelId];
    if (!chi.ch) { return; }
    chi.volume = chi.volumeNext = volume;
}

void Mixer::service() {
    std::scoped_lock lk(playMutex_);
    const auto now = SDL_GetTicks();
    for (auto &chi : channels_) {
        if (chi.ended) {
            chi.reset();
            continue;
        }
        if (chi.fadeOut) {
            const auto delta = std::uint32_t(std::int32_t(now - chi.fadeOutStart));
            if (delta >= chi.fadeOut) {
                if (chi.chNext) {
                    chi.ch = std::move(chi.chNext);
                    chi.volume = chi.fadeInNext ? 0 : chi.volumeNext;
                    chi.fadeInStart = chi.fadeInNext ? now : 0;
                    chi.fadeIn = chi.fadeInNext;
                    chi.fadeInNext = 0;
                    chi.ch->setRepeat(chi.repeatNext);
                    chi.ended = false;
                    chi.sourceEnded = false;
                    chi.readyPos = chi.readySize = 0;
                    prepareChannelLocked(chi);
                    fillChannelLocked(chi);
                } else {
                    chi.reset();
                }
                chi.fadeOutStart = chi.fadeOut = 0;
            } else {
                chi.volume = int(chi.fadeOutVolumeStart
                                  * (chi.fadeOut - delta) / chi.fadeOut);
                continue;
            }
        }
        if (chi.fadeIn) {
            const auto delta = std::uint32_t(std::int32_t(now - chi.fadeInStart));
            if (delta >= chi.fadeIn) {
                chi.fadeInStart = chi.fadeIn = 0;
            } else {
                chi.volume = int(chi.volumeNext * delta / chi.fadeIn);
            }
        }
        fillChannelLocked(chi);
    }
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
    if (!mixer || !stream || len <= 0) { return; }
    std::scoped_lock lk(mixer->playMutex_);
    auto &channels = mixer->channels_;
    if (channels.empty()) {
        memset(stream, 0, static_cast<size_t>(len));
        return;
    }
    memset(stream, 0, len);
    for (auto &chi: channels) {
        if (!chi.ch || chi.ended) { continue; }
        const auto rsize = std::min<std::size_t>(chi.readySize,
                                                 static_cast<std::size_t>(len));
        if (rsize && chi.volume) {
            SDL_MixAudioFormat(stream, chi.ready.data() + chi.readyPos,
                               mixer->format_, rsize, chi.volume);
        }
        chi.readyPos += rsize;
        chi.readySize -= rsize;
        if (chi.readySize == 0) {
            chi.readyPos = 0;
            if (chi.sourceEnded) {
                chi.ended = true;
            }
        }
        if (rsize == 0 && chi.sourceEnded) {
            chi.ended = true;
        }
    }
}

}
