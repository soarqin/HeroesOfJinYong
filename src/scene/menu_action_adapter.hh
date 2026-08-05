#pragma once

#include "logic/menu.hh"

#include <functional>
#include <utility>

namespace hojy::scene {

/**
 * Adapter kept at the scene-controller boundary.  Menu widgets only know
 * MenuSelectionSink; business controllers may still compose small closures
 * while their concrete action classes are being migrated.
 */
class LambdaMenuAction final: public MenuAction {
public:
    template<typename Function>
    explicit LambdaMenuAction(Function function)
        : function_(std::move(function)) {}

    void execute(MenuSelection selection) override {
        if (function_) { function_(std::move(selection)); }
    }

private:
    std::function<void(MenuSelection)> function_;
};

template<typename Function>
std::unique_ptr<MenuAction> makeMenuAction(Function function) {
    return std::make_unique<LambdaMenuAction>(std::move(function));
}

}
