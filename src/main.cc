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

#include <iostream>

#include "core/config.hh"
#include "data/loader.hh"
#include "scene/window.hh"
#include "audio/mixer.hh"
#include "audio/channelwav.hh"
#include "audio/channelmidi.hh"

using namespace hojy;

int main() {
    core::config.load("config.toml");
    data::loadData();
    scene::Window win(640, 480);
    audio::Mixer mixer;
    /*
    auto *t1 = new audio::ChannelWav(&mixer, "test.wav");
    t1->setRepeat(true);
    mixer.play(0, t1);
    mixer.play(1, new audio::ChannelWav(&mixer, "test2.wav"));
     */
    auto *midi = new audio::ChannelMIDI(&mixer, "data/GAME01.XMI");
    midi->setRepeat(true);
    mixer.play(0, midi);
    mixer.pause(false);
    while (win.processEvents()) {
        win.render();
        win.flush();
    }
    return 0;
}
