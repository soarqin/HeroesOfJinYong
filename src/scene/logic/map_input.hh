#pragma once

#include "input.hh"

#include <memory>

namespace hojy::scene {

class MapInputContext {
public:
    virtual ~MapInputContext() = default;
    virtual void requestMove(InputKey key) = 0;
    virtual void requestInteract() = 0;
    virtual void requestOpenMenu() = 0;
};

class MapInputAction {
public:
    virtual ~MapInputAction() = default;
    virtual void execute(MapInputContext &context) const = 0;
};

class MapMoveAction final : public MapInputAction {
public:
    explicit MapMoveAction(InputKey key): key_(key) {}
    void execute(MapInputContext &context) const override {
        context.requestMove(key_);
    }

private:
    InputKey key_ = InputKey::None;
};

class MapInteractAction final : public MapInputAction {
public:
    void execute(MapInputContext &context) const override {
        context.requestInteract();
    }
};

class MapOpenMenuAction final : public MapInputAction {
public:
    void execute(MapInputContext &context) const override {
        context.requestOpenMenu();
    }
};

class MapInputMode {
public:
    virtual ~MapInputMode() = default;
    virtual std::unique_ptr<MapInputAction> translate(InputKey key) const = 0;
};

class DefaultMapInputMode final : public MapInputMode {
public:
    std::unique_ptr<MapInputAction> translate(InputKey key) const override;
};

}
