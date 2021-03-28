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

#include "channel.hh"

namespace hojy::audio {

class ChannelMIDI final: public Channel {
public:
    ChannelMIDI(Mixer *mixer, const std::string &filename);
    ChannelMIDI(Mixer *mixer, const void *data, size_t size);
    ~ChannelMIDI() override;

    void load(const std::string &filename) override;
    void reset() override;
    void setRepeat(bool r) override;

protected:
    size_t readPCMData(const void **data, size_t size) override;

private:
    void loadFromData();

private:
    void *midiplayer_ = nullptr;
    std::vector<short> cache_;
};

}
