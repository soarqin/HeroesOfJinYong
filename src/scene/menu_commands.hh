#pragma once

#include "logic/command.hh"
#include "node.hh"

#include <cstdint>
#include <memory>

namespace hojy::scene {

class MenuOption;

class MedicActionCommand final: public SceneCommand {
public:
    MedicActionCommand(std::int16_t actorId, std::int16_t targetId,
                       std::int16_t stamina) noexcept;
    void execute(SceneCommandContext &context) override;

private:
    std::int16_t actorId_ = -1;
    std::int16_t targetId_ = -1;
    std::int16_t stamina_ = 0;
};

class DepoisonActionCommand final: public SceneCommand {
public:
    DepoisonActionCommand(std::int16_t actorId, std::int16_t targetId,
                          std::int16_t stamina) noexcept;
    void execute(SceneCommandContext &context) override;

private:
    std::int16_t actorId_ = -1;
    std::int16_t targetId_ = -1;
    std::int16_t stamina_ = 0;
};

class LeaveTeamActionCommand final: public SceneCommand {
public:
    explicit LeaveTeamActionCommand(std::int16_t characterId) noexcept;
    void execute(SceneCommandContext &context) override;

private:
    std::int16_t characterId_ = -1;
};

class PurchaseShopOfferCommand final: public SceneCommand {
public:
    PurchaseShopOfferCommand(std::int16_t shopId, std::int16_t slot) noexcept;
    void execute(SceneCommandContext &context) override;

private:
    std::int16_t shopId_ = -1;
    std::int16_t slot_ = -1;
};

class OptionsCommitCommand final: public SceneCommand {
public:
    OptionsCommitCommand(Node *menu, OptionsCommitRequest request);
    void execute(SceneCommandContext &context) override;

private:
    void updateMenuValue(int value);

    Node::LifetimeHandle menuLifetime_;
    OptionsCommitRequest request_;
};

class ContinueEventCommand final: public SceneCommand {
public:
    explicit ContinueEventCommand(bool result) noexcept: result_(result) {}
    void execute(SceneCommandContext &context) override {
        context.continueEvent(result_);
    }

private:
    bool result_ = false;
};

}
