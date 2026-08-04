#include "window.hh"

#include "audio/mixer.hh"
#include "core/config.hh"

#include <fmt/format.h>

namespace hojy::scene {

void Window::playMusic(int idx) {
    ++idx;
    if (playingMusic_ == idx) {
        return;
    }
    audio::gMixer.play(0,
                       core::config.musicFilePath(fmt::format("GAME{:02}.XMI", idx)),
                       true,
                       16 * core::config.musicVolume(),
                       500,
                       2000);
    playingMusic_ = idx;
}

void Window::playAtkSound(int idx) {
    if (idx >= 24) {
        playEffectSound(idx - 24);
        return;
    }
    audio::gMixer.play(1,
                       core::config.soundFilePath(fmt::format("ATK{:02}.WAV", idx)),
                       false,
                       16 * core::config.soundVolume());
}

void Window::playEffectSound(int idx) {
    audio::gMixer.play(2,
                       core::config.soundFilePath(fmt::format("E{:02}.WAV", idx)),
                       false,
                       16 * core::config.soundVolume());
}

}
