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

#pragma once

#include <mutex>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>

namespace hojy::audio {

class Channel;

class Mixer final {
    struct ChannelInfo {
        ChannelInfo() = default;
        ChannelInfo(const ChannelInfo&) = delete;
        ChannelInfo(ChannelInfo&&) noexcept = default;
        ChannelInfo& operator=(ChannelInfo&&) = default;
        std::unique_ptr<Channel> ch;
        int volume = 0;
        std::uint32_t fadeInStart = 0, fadeIn = 0;
        std::uint32_t fadeOutStart = 0, fadeOut = 0;
        int fadeOutVolumeStart = 0;
        std::unique_ptr<Channel> chNext;
        int volumeNext = 0;
        std::uint32_t fadeInNext = 0;
        bool repeatNext = false;
        bool ended = false;
        bool sourceEnded = false;
        std::vector<std::uint8_t> ready;
        std::size_t readyPos = 0;
        std::size_t readySize = 0;

        void reset();
    };
public:
    enum DataType {
        InvalidType = -1, F32 = 0,  F64,  I32,  I16,
    };
    enum {
        VolumeMax = 128,
    };

    ~Mixer();
    [[nodiscard]] bool init(int channels);

    void play(size_t channelId, Channel *ch, int volume = VolumeMax, std::uint32_t fadeOutMs = 0, std::uint32_t fadeInMs = 0);
    void play(size_t channelId, const std::string &filename, bool repeat, int volume = VolumeMax, std::uint32_t fadeOutMs = 0, std::uint32_t fadeInMs = 0);
    void pause(bool on) const;
    [[nodiscard]] inline std::uint32_t sampleRate() const { return sampleRate_; }
    [[nodiscard]] inline DataType dataType() const { return convertDataType(format_); }
    void setVolume(size_t channelId, int volume);
    // Main-thread maintenance for fades, file loading and channel cleanup.
    void service();

    static DataType convertDataType(std::uint16_t type);
    static std::uint16_t convertType(DataType);
    static size_t dataTypeToSize(Mixer::DataType type);

private:
    static void callback(void *userdata, std::uint8_t *stream, int len);
    void prepareChannelLocked(ChannelInfo &channel);
    void fillChannelLocked(ChannelInfo &channel);
    std::unique_ptr<Channel> createChannelLocked(const std::string &filename);
    bool loadFilenameLocked(ChannelInfo &channel, const std::string &filename,
                            bool repeat, int volume);

private:
    std::uint32_t audioDevice_ = 0;
    std::uint32_t sampleRate_ = 0;
    std::uint16_t format_ = 0;
    std::vector<ChannelInfo> channels_;
    std::vector<std::uint8_t> cache_;
    mutable std::mutex playMutex_;
};

extern Mixer gMixer;

}
