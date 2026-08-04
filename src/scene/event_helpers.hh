#pragma once

#include <cstddef>

namespace hojy::scene::detail {

inline bool resolveDeferredBranchStart(bool started,
                                       std::size_t &index,
                                       std::size_t &successAdvance,
                                       std::size_t &failureAdvance) {
    if (started) { return true; }
    index += failureAdvance;
    successAdvance = 0;
    failureAdvance = 0;
    return false;
}

}
