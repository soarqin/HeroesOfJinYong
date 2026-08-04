#pragma once

#include <utility>

namespace hojy::scene::detail {

template<typename Pointer, typename Function>
void invokeIfPresent(Pointer pointer, Function &&function) {
    if (pointer != nullptr) {
        std::forward<Function>(function)(*pointer);
    }
}

}
