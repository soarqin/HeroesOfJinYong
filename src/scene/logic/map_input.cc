#include "map_input.hh"

#include <array>
#include <cstddef>
#include <memory>

namespace hojy::scene {
namespace {

using Factory = std::unique_ptr<MapInputAction>(*)();

std::unique_ptr<MapInputAction> makeMoveUp() {
    return std::make_unique<MapMoveAction>(InputKey::Up);
}

std::unique_ptr<MapInputAction> makeMoveDown() {
    return std::make_unique<MapMoveAction>(InputKey::Down);
}

std::unique_ptr<MapInputAction> makeMoveLeft() {
    return std::make_unique<MapMoveAction>(InputKey::Left);
}

std::unique_ptr<MapInputAction> makeMoveRight() {
    return std::make_unique<MapMoveAction>(InputKey::Right);
}

std::unique_ptr<MapInputAction> makeInteract() {
    return std::make_unique<MapInteractAction>();
}

std::unique_ptr<MapInputAction> makeOpenMenu() {
    return std::make_unique<MapOpenMenuAction>();
}

std::unique_ptr<MapInputAction> makeNone() {
    return nullptr;
}

const std::array<Factory, 9> &factories() {
    static const std::array<Factory, 9> value = {
        &makeNone,
        &makeMoveUp,
        &makeMoveDown,
        &makeMoveLeft,
        &makeMoveRight,
        &makeInteract,
        &makeOpenMenu,
        &makeInteract,
        &makeNone,
    };
    return value;
}

}

std::unique_ptr<MapInputAction>
DefaultMapInputMode::translate(InputKey key) const {
    const auto index = static_cast<std::size_t>(key);
    const auto &table = factories();
    if (index >= table.size()) { return nullptr; }
    return table[index]();
}

}
