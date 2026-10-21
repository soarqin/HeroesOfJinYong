#include "transaction.hh"

namespace hojy::world::state {
namespace {
std::uint64_t revision = 1;
}

std::uint64_t stateRevision() noexcept {
    return revision;
}

void bumpStateRevision() noexcept {
    ++revision;
    if (revision == 0) { revision = 1; }
}

}
