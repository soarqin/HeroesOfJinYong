#include "window.hh"

#include "charlistmenu.hh"
#include "character_list_snapshot_builder.hh"
#include "item_snapshot_builder.hh"
#include "itemview.hh"
#include "item_selection_controller.hh"
#include "menu.hh"
#include "menu_action_adapter.hh"
#include "menu_commands.hh"
#include "window_command.hh"
#include "statusview.hh"
#include "status_snapshot_builder.hh"
#include "talkbox.hh"

#include "content/constants.hh"
#include "content/event.hh"
#include "core/config.hh"
#include "world/item_transaction.hh"
#include "world/menu_transaction.hh"
#include "world/strings.hh"

#include <fmt/xchar.h>

#include <memory>
#include <optional>
#include <utility>

namespace hojy::scene {
namespace {

enum MainMenuEntry : std::int32_t {
    MainMedic = 100,
    MainDepoison = 101,
    MainItems = 102,
    MainStatus = 103,
    MainLeaveTeam = 104,
    MainSystem = 105,
};

enum SystemMenuEntry : std::int32_t {
    SystemLoad = 200,
    SystemSave = 201,
    SystemOptions = 202,
    SystemQuit = 203,
};

enum OptionMenuEntry : std::int32_t {
    OptionMiniPanel = 300,
    OptionMinimap = 301,
    OptionMusic = 302,
    OptionSound = 303,
};

void medicMenu(Window *window, Node *mainMenu);
void medicTargetMenu(Window *window, Node *mainMenu, std::int16_t charId);
void depoisonMenu(Window *window, Node *mainMenu);
void depoisonTargetMenu(Window *window, Node *mainMenu, std::int16_t charId);
void showItems(Window *window, Node *mainMenu);
void statusMenu(Window *window, Node *mainMenu);
void showCharStatus(Window *window, Node *parent, std::int16_t charId);
void leaveTeamMenu(Window *window, Node *mainMenu);
void systemMenu(Window *window, Node *mainMenu);
void selectSaveSlotMenu(Window *window, Node *mainMenu, int x, int y, bool isSave);
void optionMenu(Window *window, Node *mainMenu, int x, int y);

template<typename Function>
void enqueueScene(Node *node, Function function) {
    if (!node) { return; }
    node->postCommand(
        [function = std::move(function)](SceneCommandContext &context) mutable {
            function(context);
        });
}

std::shared_ptr<ActionMenuController> controllerFor(
        Node *menu, bool deleteOnCancel = true) {
    auto controller = std::make_shared<ActionMenuController>();
    if (deleteOnCancel) {
        controller->bindCancel(makeMenuAction(
            [menu](MenuSelection) { if (menu) { menu->requestDelete(); } }));
    }
    return controller;
}

void medicMenu(Window *window, Node *mainMenu) {
    const auto x = mainMenu->x() + mainMenu->width()
        + core::config.windowBorder();
    const auto y = mainMenu->y();
    auto *menu = new CharListMenu(
        mainMenu, x, y, mainMenu->rootWidth() - x, mainMenu->rootHeight() - y);
    auto controller = controllerFor(menu);
    controller->bindDefault(makeMenuAction(
        [window, mainMenu](MenuSelection selection) {
            if (selection.gesture == MenuGesture::Activate) {
                medicTargetMenu(window, mainMenu,
                                static_cast<std::int16_t>(selection.entryId));
            }
        }));
    menu->init(buildCharacterListSnapshot(
                   {GETTEXT(53)}, teamCharacterSources(),
                   {medicProjection(1)}), std::move(controller));
}

void medicTargetMenu(Window *, Node *mainMenu, std::int16_t charId) {
    const auto x = mainMenu->x() + mainMenu->width()
        + core::config.windowBorder() * 3;
    const auto y = mainMenu->y() + core::config.windowBorder() * 2;
    auto *menu = new CharListMenu(
        mainMenu, x, y, mainMenu->rootWidth() - x, mainMenu->rootHeight() - y);
    auto controller = controllerFor(menu);
    controller->bindDefault(makeMenuAction(
        [mainMenu, charId](MenuSelection selection) {
            if (selection.gesture != MenuGesture::Activate) { return; }
            postOwnedSceneCommand(mainMenu,
                std::make_unique<MedicActionCommand>(
                    charId, static_cast<std::int16_t>(selection.entryId), 2));
        }));
    menu->init(buildCharacterListSnapshot(
                   {GETTEXT(54)}, teamCharacterSources(),
                   {healthProjection()}), std::move(controller));
}

void depoisonMenu(Window *window, Node *mainMenu) {
    const auto x = mainMenu->x() + mainMenu->width()
        + core::config.windowBorder();
    const auto y = mainMenu->y();
    auto *menu = new CharListMenu(
        mainMenu, x, y, mainMenu->rootWidth() - x, mainMenu->rootHeight() - y);
    auto controller = controllerFor(menu);
    controller->bindDefault(makeMenuAction(
        [window, mainMenu](MenuSelection selection) {
            if (selection.gesture == MenuGesture::Activate) {
                depoisonTargetMenu(window, mainMenu,
                                   static_cast<std::int16_t>(selection.entryId));
            }
        }));
    menu->init(buildCharacterListSnapshot(
                   {GETTEXT(56)}, teamCharacterSources(),
                   {depoisonProjection(1)}), std::move(controller));
}

void depoisonTargetMenu(Window *, Node *mainMenu, std::int16_t charId) {
    const auto x = mainMenu->x() + mainMenu->width()
        + core::config.windowBorder() * 3;
    const auto y = mainMenu->y() + core::config.windowBorder() * 2;
    auto *menu = new CharListMenu(
        mainMenu, x, y, mainMenu->rootWidth() - x, mainMenu->rootHeight() - y);
    auto controller = controllerFor(menu);
    controller->bindDefault(makeMenuAction(
        [mainMenu, charId](MenuSelection selection) {
            if (selection.gesture != MenuGesture::Activate) { return; }
            postOwnedSceneCommand(mainMenu,
                std::make_unique<DepoisonActionCommand>(
                    charId, static_cast<std::int16_t>(selection.entryId), 2));
        }));
    menu->init(buildCharacterListSnapshot(
                   {GETTEXT(57)}, teamCharacterSources(),
                   {healthProjection()}), std::move(controller));
}

void showItems(Window *window, Node *mainMenu) {
    const auto x = mainMenu->x() + mainMenu->width()
        + core::config.windowBorder();
    const auto y = mainMenu->y();
    const auto border = core::config.windowBorder();
    auto *view = new ItemView(
        mainMenu, x, y, mainMenu->rootWidth() - x - border * 4,
        mainMenu->rootHeight() - y - border * 4);
    const auto itemSnapshot = ::hojy::world::state::itemSelectionSnapshot();
    std::optional<std::pair<int, int>> compass;
    if (window && window->globalMap()) {
        compass = std::make_pair(window->globalMap()->currX(),
                                 window->globalMap()->currY());
    }
    view->show(
        buildItemViewSnapshot(itemSnapshot.bagItems, compass),
        std::make_unique<WorldItemSelectionController>(
            compass,
            [](ItemSelectionHost &host) { host.closeItemSelection(); },
            [](ItemSelectionHost &host, std::int16_t itemId) {
                host.closeItemSelection();
                host.useQuestItem(itemId);
            }));
}

void statusMenu(Window *window, Node *mainMenu) {
    const auto x = mainMenu->x() + mainMenu->width()
        + core::config.windowBorder();
    const auto y = mainMenu->y();
    auto *menu = new CharListMenu(
        mainMenu, x, y, mainMenu->rootWidth() - x, mainMenu->rootHeight() - y);
    auto controller = controllerFor(menu);
    controller->bindDefault(makeMenuAction(
        [window, mainMenu](MenuSelection selection) {
            if (selection.gesture == MenuGesture::Activate) {
                showCharStatus(window, mainMenu,
                               static_cast<std::int16_t>(selection.entryId));
            }
        }));
    menu->init(buildCharacterListSnapshot(
                   {GETTEXT(59)}, teamCharacterSources(),
                   {levelProjection()}), std::move(controller));
}

void showCharStatus(Window *window, Node *parent, std::int16_t charId) {
    const auto x = parent->x() + parent->width()
        + core::config.windowBorder();
    const auto y = parent->y();
    auto *view = new StatusView(
        parent, x, y, parent->rootWidth() - x, parent->rootHeight() - y);
    if (window) {
        view->setHeadTextureProvider(
            [window](std::int16_t id) { return window->headTexture(id); });
    }
    auto snapshot = buildCharacterStatusSnapshot(
        charId, false, core::config.showPotential());
    if (!snapshot) {
        view->requestDelete();
        return;
    }
    view->show(std::move(*snapshot));
}

void leaveTeamMenu(Window *, Node *mainMenu) {
    const auto x = mainMenu->x() + mainMenu->width()
        + core::config.windowBorder();
    const auto y = mainMenu->y();
    auto *menu = new CharListMenu(
        mainMenu, x, y, mainMenu->rootWidth() - x, mainMenu->rootHeight() - y);
    auto controller = controllerFor(menu);
    controller->bindDefault(makeMenuAction(
        [mainMenu](MenuSelection selection) {
            if (selection.gesture != MenuGesture::Activate) { return; }
            const auto charId = static_cast<std::int16_t>(selection.entryId);
            if (charId == 0) {
                enqueueScene(mainMenu, [](SceneCommandContext &context) {
                    context.showMessage(
                        {GETTEXT(61)}, ScenePopupType::PressToCloseThis);
                });
                return;
            }
            postOwnedSceneCommand(mainMenu,
                std::make_unique<LeaveTeamActionCommand>(charId));
        }));
    menu->init(buildCharacterListSnapshot(
                   {GETTEXT(60)}, teamCharacterSources(),
                   {levelProjection()}), std::move(controller));
}

void systemMenu(Window *window, Node *mainMenu) {
    const auto x = mainMenu->x() + mainMenu->width()
        + core::config.windowBorder();
    const auto y = mainMenu->y();
    auto *subMenu = new MenuTextList(
        mainMenu, x, y, mainMenu->rootWidth() - x, mainMenu->rootHeight() - y);
    auto entries = MenuEntries{
        {SystemLoad, GETTEXT(62), L"", true},
        {SystemSave, GETTEXT(63), L"", true},
        {SystemOptions, GETTEXT(131), L"", true},
        {SystemQuit, GETTEXT(64), L"", true},
    };
    subMenu->popup(entries);
    auto controller = controllerFor(subMenu);
    controller->bind(SystemLoad, makeMenuAction(
        [window, mainMenu, subMenu, y](MenuSelection) {
            selectSaveSlotMenu(window, mainMenu,
                               subMenu->x() + subMenu->width()
                                   + core::config.windowBorder(), y, false);
        }));
    controller->bind(SystemSave, makeMenuAction(
        [window, mainMenu, subMenu, y](MenuSelection) {
            selectSaveSlotMenu(window, mainMenu,
                               subMenu->x() + subMenu->width()
                                   + core::config.windowBorder(), y, true);
        }));
    controller->bind(SystemOptions, makeMenuAction(
        [window, mainMenu, subMenu, y](MenuSelection) {
            optionMenu(window, mainMenu,
                       subMenu->x() + subMenu->width()
                           + core::config.windowBorder(), y);
        }));
    controller->bind(SystemQuit, makeMenuAction(
        [mainMenu, subMenu, y](MenuSelection) {
            const auto x = subMenu->x() + subMenu->width()
                + core::config.windowBorder();
            auto *yesNo = new MenuYesNo(
                mainMenu, x, y, mainMenu->rootWidth() - x,
                mainMenu->rootHeight() - y);
            yesNo->enableHorizonal(true);
            yesNo->popupWithYesNo();
            auto choice = controllerFor(yesNo);
            choice->bind(0, makeMenuAction(
                [mainMenu](MenuSelection) {
                    enqueueScene(mainMenu, [](SceneCommandContext &context) {
                        context.forceQuit();
                    });
                }));
            yesNo->setSelectionSink(std::move(choice));
        }));
    subMenu->setSelectionSink(std::move(controller));
}

void selectSaveSlotMenu(Window *, Node *mainMenu, int x, int y, bool isSave) {
    auto *subMenu = new MenuTextList(
        mainMenu, x, y, mainMenu->rootWidth() - x, mainMenu->rootHeight() - y);
    subMenu->popup(MenuEntries{
        {1, GETTEXT(65), L"", true},
        {2, GETTEXT(66), L"", true},
        {3, GETTEXT(67), L"", true},
    });
    auto controller = controllerFor(subMenu);
    for (int slot = 1; slot <= 3; ++slot) {
        controller->bind(slot, makeMenuAction(
            [subMenu, slot, isSave](MenuSelection) {
                enqueueScene(subMenu, [slot, isSave](SceneCommandContext &context) {
                    if (isSave) {
                        context.saveGame(slot);
                        context.showMessage(
                            {GETTEXT(68)}, ScenePopupType::PressToCloseTop);
                    } else if (context.loadGame(slot)) {
                        context.closePopup();
                    } else {
                        context.showMessage(
                            {GETTEXT(69)}, ScenePopupType::PressToCloseTop);
                    }
                });
            }));
    }
    subMenu->setSelectionSink(std::move(controller));
}

void optionMenu(Window *, Node *mainMenu, int x, int y) {
    auto *subMenu = new MenuOption(
        mainMenu, x, y, mainMenu->rootWidth() - x, mainMenu->rootHeight() - y);
    subMenu->popup(MenuEntries{
        {OptionMiniPanel, GETTEXT(132), fmt::format(
            L" {:<2}", core::config.showMapMiniPanel()
                ? GETTEXT(135) : GETTEXT(136)), true},
        {OptionMinimap, GETTEXT(137), fmt::format(
            L" {:<2}", core::config.showMinimap()
                ? GETTEXT(135) : GETTEXT(136)), true},
        {OptionMusic, GETTEXT(133), fmt::format(
            L" {:>2}", core::config.musicVolume()), true},
        {OptionSound, GETTEXT(134), fmt::format(
            L" {:>2}", core::config.soundVolume()), true},
    });
    subMenu->enableHorizonal(false);
    auto controller = controllerFor(subMenu);
    controller->bindCancel(makeMenuAction(
        [subMenu](MenuSelection) {
            subMenu->postCommand(std::make_unique<OptionsCommitCommand>(
                nullptr, OptionsCommitRequest{OptionCommandId::Save,
                                               OptionAdjustment::None}));
            subMenu->requestDelete();
        }));
    controller->bindDefault(makeMenuAction(
        [subMenu](MenuSelection selection) {
            const auto id = selection.entryId;
            if (id == OptionMiniPanel && selection.gesture == MenuGesture::Activate) {
                postOwnedSceneCommand(subMenu,
                    std::make_unique<OptionsCommitCommand>(
                        subMenu, OptionsCommitRequest{
                            OptionCommandId::MiniPanel,
                            OptionAdjustment::None}));
            } else if (id == OptionMinimap
                       && selection.gesture == MenuGesture::Activate) {
                postOwnedSceneCommand(subMenu,
                    std::make_unique<OptionsCommitCommand>(
                        subMenu, OptionsCommitRequest{
                            OptionCommandId::Minimap,
                            OptionAdjustment::None}));
            } else if (id == OptionMusic
                       && (selection.gesture == MenuGesture::AdjustPrevious
                           || selection.gesture == MenuGesture::AdjustNext)) {
                postOwnedSceneCommand(subMenu,
                    std::make_unique<OptionsCommitCommand>(
                        subMenu, OptionsCommitRequest{
                            OptionCommandId::MusicVolume,
                            selection.gesture == MenuGesture::AdjustNext
                                ? OptionAdjustment::Next
                                : OptionAdjustment::Previous}));
            } else if (id == OptionSound
                       && (selection.gesture == MenuGesture::AdjustPrevious
                           || selection.gesture == MenuGesture::AdjustNext)) {
                postOwnedSceneCommand(subMenu,
                    std::make_unique<OptionsCommitCommand>(
                        subMenu, OptionsCommitRequest{
                            OptionCommandId::SoundVolume,
                            selection.gesture == MenuGesture::AdjustNext
                                ? OptionAdjustment::Next
                                : OptionAdjustment::Previous}));
            }
        }));
    subMenu->setSelectionSink(std::move(controller));
}

}

void Window::showMainMenu(bool inSubMap) {
    (void)inSubMap;
    if (popup_) { return; }
    if (mainMenu_ == nullptr) {
        const auto border = core::config.windowBorder();
        auto *menu = new MenuTextList(
            renderer_, 4 * border, 4 * border, width_ - 80, height_ - 80);
        bindCommandSink(menu);
        mainMenu_ = menu;
    }
    auto *menu = dynamic_cast<MenuTextList *>(mainMenu_);
    if (!menu) { return; }
    auto controller = controllerFor(menu);
    controller->bind(MainMedic, makeMenuAction(
        [this](MenuSelection) { medicMenu(this, mainMenu_); }));
    controller->bind(MainDepoison, makeMenuAction(
        [this](MenuSelection) { depoisonMenu(this, mainMenu_); }));
    controller->bind(MainItems, makeMenuAction(
        [this](MenuSelection) { showItems(this, mainMenu_); }));
    controller->bind(MainStatus, makeMenuAction(
        [this](MenuSelection) { statusMenu(this, mainMenu_); }));
    controller->bind(MainLeaveTeam, makeMenuAction(
        [this](MenuSelection) { leaveTeamMenu(this, mainMenu_); }));
    controller->bind(MainSystem, makeMenuAction(
        [this](MenuSelection) { systemMenu(this, mainMenu_); }));
    controller->bindCancel(makeMenuAction(
        [this](MenuSelection) { closePopup(); }));
    menu->setSelectionSink(std::move(controller));
    if (popup_ != mainMenu_) { replacePopup(mainMenu_, false); }
    menu->popup(MenuEntries{
        {MainMedic, GETTEXT(47), L"", true},
        {MainDepoison, GETTEXT(48), L"", true},
        {MainItems, GETTEXT(49), L"", true},
        {MainStatus, GETTEXT(50), L"", true},
        {MainLeaveTeam, GETTEXT(51), L"", true},
        {MainSystem, GETTEXT(52), L"", true},
    });
}

bool Window::runShop(std::int16_t id) {
    const auto snapshot = ::hojy::world::state::shopSnapshot(id);
    if (!snapshot || snapshot->listings.empty()) { return false; }
    auto *subMenu = new MenuTextList(popup_, 0, 0, width_, height_);
    MenuEntries entries;
    entries.reserve(snapshot->listings.size());
    for (const auto &listing: snapshot->listings) {
        entries.push_back({listing.slot, listing.name,
                           std::to_wstring(listing.price), true});
    }
    subMenu->popup(entries);
    subMenu->makeCenter(width_, height_, 0, 0);
    const auto shopId = snapshot->shopId;
    auto controller = controllerFor(subMenu);
    for (const auto &listing: snapshot->listings) {
        controller->bind(listing.slot, makeMenuAction(
            [subMenu, shopId, slot = listing.slot](MenuSelection) {
                postOwnedSceneCommand(subMenu,
                    std::make_unique<PurchaseShopOfferCommand>(shopId, slot));
            }));
    }
    controller->bindCancel(makeMenuAction(
        [subMenu](MenuSelection) {
            subMenu->postCommand(std::make_unique<ContinueEventCommand>(false));
            subMenu->requestDelete();
        }));
    subMenu->setSelectionSink(std::move(controller));
    return true;
}

void Window::popupMessageBox(
        const std::vector<std::wstring> &text, MessageBox::Type type) {
    MessageBox *messageBox;
    if (popup_) {
        messageBox = new MessageBox(popup_, 0, 0, width_, height_ * 4 / 5);
    } else {
        messageBox = new MessageBox(
            renderer_, 0, 0, width_, height_ * 4 / 5);
        bindCommandSink(messageBox);
        replacePopup(messageBox, true);
    }
    messageBox->popup(text, type);
}

void Window::showMessage(std::vector<std::wstring> text, ScenePopupType type) {
    MessageBox::Type messageType = MessageBox::Normal;
    switch (type) {
    case ScenePopupType::YesNo:
        messageType = MessageBox::YesNo;
        break;
    case ScenePopupType::PressToCloseTop:
        messageType = MessageBox::PressToCloseTop;
        break;
    case ScenePopupType::PressToCloseThis:
        messageType = MessageBox::PressToCloseThis;
        break;
    case ScenePopupType::PressToCloseParent:
        messageType = MessageBox::PressToCloseParent;
        break;
    case ScenePopupType::Normal:
        break;
    }
    popupMessageBox(text, messageType);
}

void Window::setGlobalMapPosition(int x, int y) {
    if (globalMap_) { globalMap_->setPosition(x, y, false); }
}

}
