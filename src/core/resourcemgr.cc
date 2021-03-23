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

#include "resourcemgr.hh"

#include "config.hh"
#include <fmt/format.h>
#include <filesystem>

namespace hojy::core {

ResourceMgr gResourceMgr;

void ResourceMgr::init() {
    const std::set<std::string, StringCaseInsensitiveLess> dataFiles = {
        /* strings.toml */
        "strings.toml",
        /* default def file */
        "ALLDEF.GRP", "ALLDEF.IDX",
        /* default sin file */
        "ALLSIN.GRP", "ALLSIN.IDX",
        /* global map data */
        "BUILDING.002", "BUILDX.002", "BUILDY.002", "EARTH.002", "SURFACE.002",
        /* cloud textures */
        "CLOUD.GRP", "CLOUD.IDX",
        /* dead screen */
        "DEAD.BIG",
        /* effect textures */
        "EFT.GRP", "EFT.IDX",
        /* end screen */
        "ENDCOL.COL", "ENDWORD.GRP", "ENDWORD.IDX", "KEND.GRP", "KEND.IDX",
        /* head textures */
        "HDGRP.GRP", "HDGRP.IDX",
        /* event defs */
        "KDEF.GRP", "KDEF.IDX",
        /* global map data */
        "MMAP.COL", "MMAP.GRP", "MMAP.IDX",
        /* default ranger file */
        "RANGER.GRP", "RANGER.IDX",
        /* talk data */
        "TALK.GRP", "TALK.IDX",
        /* title screen */
        "TITLE.BIG", "TITLE.GRP", "TITLE.IDX",
        /* warfield data and textures */
        "WAR.STA", "WARFLD.GRP", "WARFLD.IDX",
        /* main program, we reads some important values from it */
        "Z.DAT",
    }, dataFilesOpt = {
        /* sub map textures */
        "SDX", "SMP",
        /* warfield textures */
        "WDX", "WMP",
    }, musicFiles = {
        /* music */
        "GAME01.XMI", "GAME02.XMI", "GAME03.XMI", "GAME04.XMI", "GAME05.XMI", "GAME06.XMI",
        "GAME07.XMI", "GAME08.XMI", "GAME09.XMI", "GAME10.XMI", "GAME11.XMI", "GAME12.XMI",
        "GAME13.XMI", "GAME14.XMI", "GAME15.XMI", "GAME16.XMI", "GAME17.XMI", "GAME18.XMI",
        "GAME19.XMI", "GAME20.XMI", "GAME21.XMI", "GAME22.XMI", "GAME23.XMI", "GAME24.XMI",
    }, soundFiles = {
        /* sound */
        "ATK00.WAV", "ATK01.WAV", "ATK02.WAV", "ATK03.WAV", "ATK04.WAV", "ATK05.WAV", "ATK06.WAV",
        "ATK07.WAV", "ATK08.WAV", "ATK09.WAV", "ATK10.WAV", "ATK11.WAV", "ATK12.WAV", "ATK13.WAV",
        "ATK14.WAV", "ATK15.WAV", "ATK16.WAV", "ATK17.WAV", "ATK18.WAV", "ATK19.WAV", "ATK20.WAV",
        "ATK21.WAV", "ATK22.WAV", "ATK23.WAV",
        "E00.WAV", "E01.WAV", "E02.WAV", "E03.WAV", "E04.WAV", "E05.WAV", "E06.WAV", "E07.WAV", "E08.WAV",
        "E09.WAV", "E10.WAV", "E11.WAV", "E12.WAV", "E13.WAV", "E14.WAV", "E15.WAV", "E16.WAV", "E17.WAV",
        "E18.WAV", "E19.WAV", "E20.WAV", "E21.WAV", "E22.WAV", "E23.WAV", "E24.WAV", "E25.WAV", "E26.WAV",
        "E27.WAV", "E28.WAV", "E29.WAV", "E30.WAV", "E31.WAV", "E32.WAV", "E33.WAV", "E34.WAV", "E35.WAV",
        "E36.WAV", "E37.WAV", "E38.WAV", "E39.WAV", "E40.WAV", "E41.WAV", "E42.WAV", "E43.WAV", "E44.WAV",
        "E45.WAV", "E46.WAV", "E47.WAV", "E48.WAV", "E49.WAV", "E50.WAV", "E51.WAV", "E52.WAV",
    }, saveFiles = {
        "D1.GRP", "D1.IDX", "D2.GRP", "D2.IDX", "D3.GRP", "D3.IDX",
        "R1.GRP", "R1.IDX", "R2.GRP", "R2.IDX", "R3.GRP", "R3.IDX",
        "S1.GRP", "S1.IDX", "S2.GRP", "S2.IDX", "S3.GRP", "S3.IDX",
    };
    std::set<std::string, StringCaseInsensitiveLess> texFiles;
    for (int i = 0; i < 1000; ++i) {
        texFiles.insert(fmt::format("FIGHT{:03}.IDX", i));
        texFiles.insert(fmt::format("FIGHT{:03}.GRP", i));
        texFiles.insert(fmt::format("SDX{:03}", i));
        texFiles.insert(fmt::format("SMP{:03}", i));
        texFiles.insert(fmt::format("WDX{:03}", i));
        texFiles.insert(fmt::format("WMP{:03}", i));
    }
    auto fn = [this](const std::string &path,
                     const std::set<std::string, StringCaseInsensitiveLess> &sset,
                     const std::set<std::string, StringCaseInsensitiveLess> &sset2) {
        for (auto &p: std::filesystem::directory_iterator(path)) {
            if (!p.is_regular_file()) { continue; }
            auto filename = p.path().filename().string();
            if (files_.find(filename) != files_.end()) {
                continue;
            }
            {
                auto ite = sset.find(filename);
                if (ite != sset.end()) {
                    files_[*ite] = p.path().string();
                    continue;
                }
            }
            if (!sset2.empty()) {
                auto ite = sset2.find(filename);
                if (ite != sset2.end()) {
                    files_[*ite] = p.path().string();
                    continue;
                }
            }
        }
    };
    for (auto &path: config.dataPath()) {
        fn(path, dataFiles, dataFilesOpt);
        fn(path, texFiles, {});
    }
    fn(config.musicPath(), musicFiles, {});
    fn(config.soundPath(), soundFiles, {});
    fn(config.savePath(), saveFiles, {});
    auto fn2 = [this](std::set<std::string> &missingFiles, const std::set<std::string, StringCaseInsensitiveLess> &sset) {
        for (auto &fn: sset) {
            if (files_.find(fn) == files_.end()) {
                missingFiles.insert(fn);
            }
        }
    };
    fn2(missingFiles_, dataFiles);
    fn2(missingFilesOpt_, musicFiles);
    fn2(missingFilesOpt_, soundFiles);
}

const std::string &ResourceMgr::getFilePath(const std::string &file) const {
    static const std::string dummy;
    auto ite = files_.find(file);
    if (ite == files_.end()) {
        return dummy;
    }
    return ite->second;
}

}
