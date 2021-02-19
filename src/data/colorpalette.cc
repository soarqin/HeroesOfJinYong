#include "colorpalette.hh"

#include "core/config.hh"
#include "util/file.hh"

namespace hojy::data {

ColorPalette normalPalette, endPalette;

void ColorPalette::load(const std::string &name) {
    util::File ifs;
    ifs.open(core::config.dataFilePath(name + ".COL"));
    std::uint8_t c[4] = {0, 0, 0, 0xFF};
    for (size_t i = 0; i < 256; ++i) {
        ifs.read(c, 3);
        for (int j = 0; j < 3; ++j) {
            c[j] = uint8_t(uint32_t(c[j]) * 0xFF / 0x3F);
        }
        palette_[i] = *reinterpret_cast<std::uint32_t*>(c);
    }
    palette_[0] = 0;
}

}
