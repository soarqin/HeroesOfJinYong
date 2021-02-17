#include "loader.hh"

#include "colorpalette.hh"
#include "grpdata.hh"

namespace hojy::data {

void loadData() {
    normalPalette.load("MMAP");
    grpData.load("MMAP");
}

}
