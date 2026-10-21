#include "window.hh"

#include "audio/mixer.hh"
#include "core/config.hh"

#include <algorithm>

namespace hojy::scene {

OptionsCommitResult Window::commitOptions(OptionsCommitRequest request) {
    OptionsCommitResult result;
    switch (request.id) {
    case OptionCommandId::MiniPanel:
        if (request.adjustment != OptionAdjustment::None) { return result; }
        core::config.setShowMapMiniPanel(!core::config.showMapMiniPanel());
        result.applied = true;
        result.value = core::config.showMapMiniPanel() ? 1 : 0;
        break;
    case OptionCommandId::Minimap:
        if (request.adjustment != OptionAdjustment::None) { return result; }
        core::config.setShowMinimap(!core::config.showMinimap());
        result.applied = true;
        result.value = core::config.showMinimap() ? 1 : 0;
        break;
    case OptionCommandId::MusicVolume: {
        if (request.adjustment == OptionAdjustment::None) { return result; }
        auto value = std::clamp(
            core::config.musicVolume() + static_cast<int>(request.adjustment), 0, 8);
        core::config.setMusicVolume(value);
        audio::gMixer.setVolume(0, 16 * value);
        result.applied = true;
        result.value = value;
        break;
    }
    case OptionCommandId::SoundVolume: {
        if (request.adjustment == OptionAdjustment::None) { return result; }
        auto value = std::clamp(
            core::config.soundVolume() + static_cast<int>(request.adjustment), 0, 8);
        core::config.setSoundVolume(value);
        result.applied = true;
        result.value = value;
        break;
    }
    case OptionCommandId::Save:
        result.applied = core::config.saveOptions(
            core::config.saveFilePath("options.toml"));
        break;
    }
    return result;
}

void Window::continueEvent(bool result) {
    if (subMap_) { subMap_->continueEvents(result); }
}

}
