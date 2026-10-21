#include "menu_commands.hh"

#include "content/event.hh"
#include "menu.hh"
#include "world/menu_transaction.hh"
#include "world/strings.hh"

#include <algorithm>
#include <fmt/xchar.h>
#include <utility>

namespace hojy::scene {

MedicActionCommand::MedicActionCommand(
        std::int16_t actorId, std::int16_t targetId,
        std::int16_t stamina) noexcept:
    actorId_(actorId), targetId_(targetId), stamina_(stamina) {
}

void MedicActionCommand::execute(SceneCommandContext &context) {
    auto candidate = ::hojy::world::state::prepareMedic(
        actorId_, targetId_, stamina_);
    if (!candidate) { return; }
    const auto result = candidate->result();
    if (!::hojy::world::state::commitUtilityAction(std::move(*candidate))) {
        return;
    }
    context.closePopup();
    context.showMessage(
        {GETTEXT(55) + L' ' + std::to_wstring(result)},
        ScenePopupType::PressToCloseTop);
}

DepoisonActionCommand::DepoisonActionCommand(
        std::int16_t actorId, std::int16_t targetId,
        std::int16_t stamina) noexcept:
    actorId_(actorId), targetId_(targetId), stamina_(stamina) {
}

void DepoisonActionCommand::execute(SceneCommandContext &context) {
    auto candidate = ::hojy::world::state::prepareDepoison(
        actorId_, targetId_, stamina_);
    if (!candidate) { return; }
    const auto result = candidate->result();
    if (!::hojy::world::state::commitUtilityAction(std::move(*candidate))) {
        return;
    }
    context.closePopup();
    context.showMessage(
        {GETTEXT(58) + L' ' + std::to_wstring(result)},
        ScenePopupType::PressToCloseTop);
}

LeaveTeamActionCommand::LeaveTeamActionCommand(
        std::int16_t characterId) noexcept:
    characterId_(characterId) {
}

void LeaveTeamActionCommand::execute(SceneCommandContext &context) {
    auto candidate = ::hojy::world::state::prepareLeaveTeam(characterId_);
    if (!candidate) { return; }
    const auto eventId = candidate->eventId();
    if (!::hojy::world::state::commitUtilityAction(std::move(*candidate))) {
        return;
    }
    context.closePopup();
    if (eventId >= 0) { context.forceEvent(eventId); }
}

PurchaseShopOfferCommand::PurchaseShopOfferCommand(
        std::int16_t shopId, std::int16_t slot) noexcept:
    shopId_(shopId), slot_(slot) {
}

void PurchaseShopOfferCommand::execute(SceneCommandContext &context) {
    auto candidate = ::hojy::world::state::prepareShopPurchase(
        shopId_, slot_);
    if (!candidate
        || !::hojy::world::state::commitShopPurchase(std::move(*candidate))) {
        context.closePopup();
        context.runTalk(::hojy::content::gEvent.talk(0xB9F), 0x6F, 0);
        return;
    }
    context.closePopup();
    context.runTalk(::hojy::content::gEvent.talk(0xBA0), 0x6F, 0);
}

OptionsCommitCommand::OptionsCommitCommand(
        Node *menu, OptionsCommitRequest request):
    menuLifetime_(menu ? menu->lifetimeHandle() : Node::LifetimeHandle()),
    request_(request) {
}

void OptionsCommitCommand::execute(SceneCommandContext &context) {
    const auto result = context.commitOptions(request_);
    if (result.applied) { updateMenuValue(result.value); }
}

void OptionsCommitCommand::updateMenuValue(int value) {
    auto state = menuLifetime_.lock();
    if (!state || !state->owner) { return; }
    auto *menu = dynamic_cast<MenuOption *>(state->owner);
    if (!menu) { return; }
    switch (request_.id) {
    case OptionCommandId::MiniPanel:
        menu->setValueById(
            300, fmt::format(L" {:<2}", value != 0 ? GETTEXT(135) : GETTEXT(136)));
        break;
    case OptionCommandId::Minimap:
        menu->setValueById(
            301, fmt::format(L" {:<2}", value != 0 ? GETTEXT(135) : GETTEXT(136)));
        break;
    case OptionCommandId::MusicVolume:
        menu->setValueById(302, fmt::format(L" {:>2}", value));
        break;
    case OptionCommandId::SoundVolume:
        menu->setValueById(303, fmt::format(L" {:>2}", value));
        break;
    case OptionCommandId::Save:
        break;
    }
}

}
