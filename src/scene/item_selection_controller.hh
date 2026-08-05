#pragma once

#include "logic/presentation.hh"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hojy::scene {

using ItemSelectionEntry = std::pair<std::int16_t, std::int16_t>;

/**
 * Lifetime token for asynchronous presentation continuations owned by an
 * ItemView.  A continuation may outlive the view's node until the next
 * fixed-logic deletion barrier, but it must never dereference the old host
 * after the view has actually been destroyed.
 */
struct ItemSelectionLifetime final {
    bool alive = true;
};

/**
 * Fixed-logic host exposed to item policies.  The host is deliberately a
 * value/command adapter: policies may mutate domain state and submit typed
 * presentation requests, but cannot construct scene nodes or access a
 * renderer.
 */
class ItemSelectionHost {
public:
    virtual ~ItemSelectionHost() = default;
    virtual void closeItemSelection() = 0;
    virtual void replaceItemSelection(
            std::vector<ItemViewEntrySnapshot> items) = 0;
    virtual void showCharacterSelection(CharacterSelectionRequest request) = 0;
    virtual void showItemMessage(ItemMessageRequest request) = 0;
    virtual void useQuestItem(std::int16_t itemId) = 0;
};

class ItemSelectionController {
public:
    virtual ~ItemSelectionController() = default;
    virtual void bindLifetime(std::weak_ptr<ItemSelectionLifetime>) {}
    virtual void bindItems(const std::vector<ItemViewEntrySnapshot> &) {}
    virtual void select(ItemSelectionHost &host, std::int16_t itemId) = 0;
    virtual void cancel(ItemSelectionHost &host) = 0;
};

class WorldItemSelectionController;

class ItemSelectionAction {
public:
    virtual ~ItemSelectionAction() = default;
    virtual void execute(WorldItemSelectionController &controller,
                         ItemSelectionHost &host,
                         std::int16_t itemId) = 0;
};

class FunctionItemSelectionController final : public ItemSelectionController {
public:
    using SelectFunction = std::function<void(ItemSelectionHost &, std::int16_t)>;
    using CancelFunction = std::function<void(ItemSelectionHost &)>;

    FunctionItemSelectionController(SelectFunction selectFunction,
                                    CancelFunction cancelFunction);

    void select(ItemSelectionHost &host, std::int16_t itemId) override;
    void cancel(ItemSelectionHost &host) override;
    void bindItems(const std::vector<ItemViewEntrySnapshot> &items) override;

private:
    SelectFunction selectFunction_;
    CancelFunction cancelFunction_;
};

class WorldItemSelectionController final : public ItemSelectionController {
public:
    using CloseFunction = std::function<void(ItemSelectionHost &)>;
    using UseQuestItemFunction = std::function<void(ItemSelectionHost &, std::int16_t)>;

    WorldItemSelectionController(std::optional<std::pair<int, int>> compassPosition,
                                 CloseFunction closeFunction,
                                 UseQuestItemFunction useQuestItemFunction);

    void select(ItemSelectionHost &host, std::int16_t itemId) override;
    void cancel(ItemSelectionHost &host) override;
    void bindItems(const std::vector<ItemViewEntrySnapshot> &items) override;
    void bindLifetime(std::weak_ptr<ItemSelectionLifetime> lifetime) override {
        lifetime_ = std::move(lifetime);
        lifetimeBound_ = true;
    }
    void beginEquipSelection(ItemSelectionHost &host, std::int16_t itemId);
    void beginConsumeSelection(ItemSelectionHost &host, std::int16_t itemId);
    void beginQuestSelection(ItemSelectionHost &host, std::int16_t itemId);
    void ignoreSelection(ItemSelectionHost &host);
private:
    friend class ItemSelectionAction;
    void equipItem(ItemSelectionHost &host, std::int16_t itemId, std::int16_t charId);
    void consumeItem(ItemSelectionHost &host, std::int16_t itemId, std::int16_t charId);

    std::optional<std::pair<int, int>> compassPosition_;
    CloseFunction closeFunction_;
    UseQuestItemFunction useQuestItemFunction_;
    std::uint64_t nextContinuationToken_ = 1;
    std::weak_ptr<ItemSelectionLifetime> lifetime_;
    bool lifetimeBound_ = false;
    std::map<std::int16_t, std::unique_ptr<ItemSelectionAction>> actions_;
};

}
