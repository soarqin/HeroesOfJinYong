/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>
 */

#include "audio/channelwav.hh"
#include "audio/resampler.hh"
#include "test_support.hh"

#include <SDL.h>

#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace hojy::audio {

Mixer::~Mixer() = default;

Mixer::DataType Mixer::convertDataType(std::uint16_t type) {
    switch (type) {
    case 0:return I16;
    case AUDIO_F32:return F32;
    case AUDIO_S32:return I32;
    case AUDIO_S16:return I16;
    default:return InvalidType;
    }
}

std::uint16_t Mixer::convertType(Mixer::DataType type) {
    switch (type) {
    case I16:return AUDIO_S16;
    case I32:return AUDIO_S32;
    default:return AUDIO_F32;
    }
}

size_t Mixer::dataTypeToSize(Mixer::DataType type) {
    switch (type) {
    case F32:
    case I32:return 4;
    case F64:return 8;
    case I16:return 2;
    default:return 1;
    }
}

Resampler::Resampler(std::uint32_t channels, double sampleRateIn, double sampleRateOut,
                     Mixer::DataType typeIn, Mixer::DataType typeOut) {
    (void)channels;
    (void)sampleRateIn;
    (void)sampleRateOut;
    (void)typeIn;
    (void)typeOut;
}

Resampler::~Resampler() = default;

void Resampler::setInputCallback(InputCallback callback) {
    inputCB_ = std::move(callback);
}

size_t Resampler::read(void *data, size_t size) {
    (void)data;
    (void)size;
    return 0;
}

}

namespace {

void append16(std::vector<std::uint8_t> &data, std::uint16_t value) {
    data.push_back(static_cast<std::uint8_t>(value));
    data.push_back(static_cast<std::uint8_t>(value >> 8));
}

void append32(std::vector<std::uint8_t> &data, std::uint32_t value) {
    append16(data, static_cast<std::uint16_t>(value));
    append16(data, static_cast<std::uint16_t>(value >> 16));
}

void appendTag(std::vector<std::uint8_t> &data, const char *tag) {
    data.insert(data.end(), tag, tag + 4);
}

void writeMonoU8Wav(const std::filesystem::path &filename) {
    const std::array<std::uint8_t, 2> samples = {0, 255};
    std::vector<std::uint8_t> data;
    appendTag(data, "RIFF");
    append32(data, 36 + samples.size());
    appendTag(data, "WAVE");
    appendTag(data, "fmt ");
    append32(data, 16);
    append16(data, 1);
    append16(data, 1);
    append32(data, 8000);
    append32(data, 8000);
    append16(data, 1);
    append16(data, 8);
    appendTag(data, "data");
    append32(data, samples.size());
    data.insert(data.end(), samples.begin(), samples.end());

    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!file) { throw std::runtime_error("failed to write wav fixture"); }
}

void wavTailAndReload() {
    const auto directory = std::filesystem::temp_directory_path() / "hojy-audio-channel-tests";
    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
    std::filesystem::create_directories(directory);
    const auto valid = directory / "valid.wav";
    const auto invalid = directory / "invalid.wav";
    writeMonoU8Wav(valid);
    {
        std::ofstream file(invalid, std::ios::binary);
        file << "not a wav";
    }

    hojy::audio::Mixer mixer;
    hojy::audio::ChannelWav channel(&mixer, valid.string());
    HOJY_CHECK_EQ(channel.ok(), true);

    std::array<std::uint8_t, 16> output;
    output.fill(0xCD);
    HOJY_CHECK_EQ(channel.readData(output.data(), output.size()), 8U);
    for (size_t i = 8; i < output.size(); ++i) {
        HOJY_CHECK_EQ(output[i], 0xCD);
    }

    channel.reset();
    channel.setRepeat(true);
    std::array<std::uint8_t, 12> repeated{};
    HOJY_CHECK_EQ(channel.readData(repeated.data(), repeated.size()), repeated.size());
    for (size_t i = 0; i < 4; ++i) {
        HOJY_CHECK_EQ(repeated[i], repeated[i + 8]);
    }

    channel.load(invalid.string());
    HOJY_CHECK_EQ(channel.ok(), false);
    channel.load(valid.string());
    HOJY_CHECK_EQ(channel.ok(), true);

    std::filesystem::remove_all(directory, ec);
}

}

int main() {
    try {
        wavTailAndReload();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
