#include "character_style.hh"

namespace hojy::world::state {

std::tuple<std::uint8_t, std::uint8_t, std::uint8_t>
calcColorForMpType(std::int16_t type) {
    switch (type) {
    case 0:
        return std::make_tuple(208, 152, 208);
    case 1:
        return std::make_tuple(236, 200, 40);
    default:
        return std::make_tuple(252, 252, 252);
    }
}

}
