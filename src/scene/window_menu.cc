#include "window.hh"

#include "charlistmenu.hh"
#include "itemview.hh"
#include "menu.hh"
#include "statusview.hh"
#include "talkbox.hh"

#include "audio/mixer.hh"
#include "content/constants.hh"
#include "core/config.hh"
#include "content/event.hh"
#include "world/action.hh"
#include "world/savedata.hh"
#include "world/strings.hh"

#include <fmt/xchar.h>

namespace hojy::scene {
namespace {

void medicTargetMenu(Node *mainMenu, std::int16_t charId);
void depoisonTargetMenu(Node *mainMenu, std::int16_t charId);
void showCharStatus(Node *parent, std::int16_t charId);
void selectSaveSlotMenu(Node *mainMenu, int x, int y, bool isSave);
void optionMenu(Node *mainMenu, int x, int y);

void medicMenu(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + core::config.windowBorder();
    auto y = mainMenu->y();
    auto *menu = new CharListMenu(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    menu->initWithTeamMembers({GETTEXT(53)}, {CharListMenu::MEDIC},
                              [mainMenu](std::int16_t charId) {
                                  medicTargetMenu(mainMenu, charId);
                              }, nullptr, [](CharListMenu::ValueType, std::int16_t value) -> bool {
            return value > 0;
        });
}

void medicTargetMenu(Node *mainMenu, std::int16_t charId) {
    auto x = mainMenu->x() + mainMenu->width() + core::config.windowBorder() * 3;
    auto y = mainMenu->y() + core::config.windowBorder() * 2;
    auto *menu = new CharListMenu(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    menu->initWithTeamMembers({GETTEXT(54)}, {CharListMenu::HP},
                              [charId](std::int16_t toCharId) {
                                  int result = ::hojy::world::state::actMedic(::hojy::world::state::gSaveData.charInfo[charId],
                                                             ::hojy::world::state::gSaveData.charInfo[toCharId], 2);
                                  gWindow->closePopup();
                                  gWindow->popupMessageBox({GETTEXT(55) + L' ' + std::to_wstring(result)},
                                                           MessageBox::PressToCloseTop);
                              }, nullptr);
}

void depoisonMenu(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + core::config.windowBorder();
    auto y = mainMenu->y();
    auto *menu = new CharListMenu(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    menu->initWithTeamMembers({GETTEXT(56)}, {CharListMenu::DEPOISON},
                              [mainMenu](std::int16_t charId) {
                                  depoisonTargetMenu(mainMenu, charId);
                              }, nullptr, [](CharListMenu::ValueType, std::int16_t value) -> bool {
            return value > 0;
        });
}

void depoisonTargetMenu(Node *mainMenu, std::int16_t charId) {
    auto x = mainMenu->x() + mainMenu->width() + core::config.windowBorder() * 3;
    auto y = mainMenu->y() + core::config.windowBorder() * 2;
    auto *menu = new CharListMenu(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    menu->initWithTeamMembers({GETTEXT(57)}, {CharListMenu::HP},
                              [charId](std::int16_t toCharId) {
                                  int result = ::hojy::world::state::actDepoison(::hojy::world::state::gSaveData.charInfo[charId],
                                                                ::hojy::world::state::gSaveData.charInfo[toCharId], 2);
                                  gWindow->closePopup();
                                  gWindow->popupMessageBox({GETTEXT(58) + L' ' + std::to_wstring(result)},
                                                           MessageBox::PressToCloseTop);
                              }, nullptr);
}

void showItems(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + core::config.windowBorder();
    auto y = mainMenu->y();
    auto border = core::config.windowBorder();
    auto *view = new ItemView(mainMenu, x, y,
                              gWindow->width() - x - border * 4,
                              gWindow->height() - y - border * 4);
    view->show(false, [](std::int16_t itemId) {
        gWindow->useQuestItem(itemId);
    });
}

void statusMenu(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + core::config.windowBorder();
    auto y = mainMenu->y();
    auto *menu = new CharListMenu(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    menu->initWithTeamMembers({GETTEXT(59)}, {CharListMenu::LEVEL},
                              [mainMenu](std::int16_t charId) {
                                  showCharStatus(mainMenu, charId);
                              }, nullptr);
}

void showCharStatus(Node *parent, std::int16_t charId) {
    auto x = parent->x() + parent->width() + core::config.windowBorder();
    auto y = parent->y();
    auto *view = new StatusView(parent, x, y, gWindow->width() - x, gWindow->height() - y);
    view->show(charId);
}

void leaveTeamMenu(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + core::config.windowBorder();
    auto y = mainMenu->y();
    auto *menu = new CharListMenu(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    menu->initWithTeamMembers({GETTEXT(60)}, {CharListMenu::LEVEL},
                              [](std::int16_t charId) {
                                  if (charId == 0) {
                                      gWindow->popupMessageBox({GETTEXT(61)}, MessageBox::PressToCloseThis);
                                      return;
                                  }
                                  if (::hojy::world::state::leaveTeam(charId)) {
                                      auto eventId = ::hojy::world::state::getLeaveEventId(charId);
                                      gWindow->closePopup();
                                      if (eventId >= 0) {
                                          gWindow->forceEvent(eventId);
                                      }
                                  }
                              }, nullptr);
}

void systemMenu(Node *mainMenu) {
    auto x = mainMenu->x() + mainMenu->width() + core::config.windowBorder();
    auto y = mainMenu->y();
    auto *subMenu = new MenuTextList(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    subMenu->popup({GETTEXT(62), GETTEXT(63), GETTEXT(131), GETTEXT(64)});
    subMenu->forceUpdate();
    x += subMenu->width() + core::config.windowBorder();
    subMenu->setHandler([mainMenu, subMenu, x, y]() {
        switch (subMenu->currIndex()) {
        case 0:
            selectSaveSlotMenu(mainMenu, x, y, false);
            break;
        case 1:
            selectSaveSlotMenu(mainMenu, x, y, true);
            break;
        case 2:
            optionMenu(mainMenu, x, y);
            break;
        case 3: {
            auto *yesNo = new MenuYesNo(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
            yesNo->setHandler([]() { gWindow->forceQuit(); },
                              [yesNo]() { yesNo->requestDelete(); });
            yesNo->popupWithYesNo();
            break;
        }
        default:
            break;
        }
    }, nullptr);
}

void selectSaveSlotMenu(Node *mainMenu, int x, int y, bool isSave) {
    auto *subMenu = new MenuTextList(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    subMenu->popup({GETTEXT(65), GETTEXT(66), GETTEXT(67)});
    subMenu->setHandler([subMenu, isSave]() {
        auto index = subMenu->currIndex();
        if (isSave) {
            gWindow->saveGame(index + 1);
            gWindow->popupMessageBox({GETTEXT(68)}, MessageBox::PressToCloseTop);
        } else if (gWindow->loadGame(index + 1)) {
            gWindow->closePopup();
        } else {
            gWindow->popupMessageBox({GETTEXT(69)}, MessageBox::PressToCloseTop);
        }
    }, nullptr);
}

void optionMenu(Node *mainMenu, int x, int y) {
    auto *subMenu = new MenuOption(mainMenu, x, y, gWindow->width() - x, gWindow->height() - y);
    std::vector<std::wstring> values = {
        fmt::format(L" {:<2}", core::config.showMapMiniPanel() ? GETTEXT(135) : GETTEXT(136)),
        fmt::format(L" {:<2}", core::config.showMinimap() ? GETTEXT(135) : GETTEXT(136)),
        fmt::format(L" {:>2}", core::config.musicVolume()),
        fmt::format(L" {:>2}", core::config.soundVolume()),
    };
    subMenu->popup({GETTEXT(132), GETTEXT(137), GETTEXT(133), GETTEXT(134)}, values);
    subMenu->setHandler([subMenu](int inputType) {
        switch (inputType) {
        case 0:
            (void)core::config.saveOptions(core::config.saveFilePath("options.toml"));
            break;
        case 1:
        case 2:
            switch (subMenu->currIndex()) {
            case 2: {
                int value = core::config.musicVolume();
                if (inputType == 1) {
                    if (value <= 0) { break; }
                    --value;
                } else {
                    if (value >= 8) { break; }
                    ++value;
                }
                core::config.setMusicVolume(value);
                audio::gMixer.setVolume(0, 16 * value);
                subMenu->setValue(1, fmt::format(L" {:>2}", value));
                break;
            }
            case 3: {
                int value = core::config.soundVolume();
                if (inputType == 1) {
                    if (value <= 0) { break; }
                    --value;
                } else {
                    if (value >= 8) { break; }
                    ++value;
                }
                core::config.setSoundVolume(value);
                subMenu->setValue(2, fmt::format(L" {:>2}", value));
                break;
            }
            default:
                break;
            }
            /* fallthrough */
        case 3:
            switch (subMenu->currIndex()) {
            case 0:
                core::config.setShowMapMiniPanel(!core::config.showMapMiniPanel());
                subMenu->setValue(0,
                                  fmt::format(L" {:<2}",
                                              core::config.showMapMiniPanel() ? GETTEXT(135) : GETTEXT(136)));
                break;
            case 1:
                core::config.setShowMinimap(!core::config.showMinimap());
                subMenu->setValue(1,
                                  fmt::format(L" {:<2}",
                                              core::config.showMinimap() ? GETTEXT(135) : GETTEXT(136)));
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    });
}

}

void Window::showMainMenu(bool inSubMap) {
    if (processingStage_) {
        defer([this, inSubMap] { showMainMenu(inSubMap); });
        return;
    }
    (void)inSubMap;
    if (popup_) {
        return;
    }
    if (mainMenu_ == nullptr) {
        auto border = core::config.windowBorder();
        auto *menu = new MenuTextList(renderer_, 4 * border, 4 * border, width_ - 80, height_ - 80);
        mainMenu_ = menu;
        menu->setHandler([this]() {
            switch (dynamic_cast<Menu *>(mainMenu_)->currIndex()) {
            case 0:
                medicMenu(mainMenu_);
                break;
            case 1:
                depoisonMenu(mainMenu_);
                break;
            case 2:
                showItems(mainMenu_);
                break;
            case 3:
                statusMenu(mainMenu_);
                break;
            case 4:
                leaveTeamMenu(mainMenu_);
                break;
            case 5:
                systemMenu(mainMenu_);
                break;
            default:
                break;
            }
        }, [this]() -> bool {
            closePopup();
            return false;
        });
    }
    popup_ = mainMenu_;
    freeOnClose_ = false;
    dynamic_cast<MenuTextList *>(mainMenu_)
        ->popup({GETTEXT(47), GETTEXT(48), GETTEXT(49), GETTEXT(50), GETTEXT(51), GETTEXT(52)});
}

void Window::runTalk(const std::wstring &text, std::int16_t headId, std::int16_t position) {
    if (processingStage_) {
        defer([this, text, headId, position] { runTalk(text, headId, position); });
        return;
    }
    if (popup_) {
        auto *map = dynamic_cast<MapWithEvent *>(map_);
        if (map) { map->continueEvents(false); }
        return;
    }
    if (!talkBox_) {
        auto border = width_ / 12;
        talkBox_ = new TalkBox(renderer_, border, border, width_ - border * 2, height_ - border * 2);
    }
    dynamic_cast<TalkBox *>(talkBox_)->popup(text, headId, position);
    popup_ = talkBox_;
    freeOnClose_ = false;
}

bool Window::runShop(std::int16_t id) {
    auto *shopInfo = ::hojy::world::state::gSaveData.shopInfo[id];
    if (!shopInfo) {
        return false;
    }
    auto *subMenu = new MenuTextList(popup_, 0, 0, width_, height_);
    std::vector<std::wstring> items;
    std::vector<std::wstring> prices;
    std::vector<int> indices;
    for (int i = 0; i < content::ShopItemCount; ++i) {
        if (shopInfo->id[i] <= 0 || shopInfo->total[i] <= 0) { continue; }
        items.emplace_back(GETITEMNAME(shopInfo->id[i]));
        prices.emplace_back(std::to_wstring(shopInfo->price[i]));
        indices.emplace_back(i);
    }
    subMenu->popup(items, prices);
    subMenu->makeCenter(width_, height_, 0, 0);
    subMenu->setHandler([subMenu, shopInfo, indices]() {
        int index = subMenu->currIndex();
        if (index < 0 || index >= static_cast<int>(indices.size())) { return; }
        index = indices[index];
        const auto price = shopInfo->price[index];
        if (!::hojy::world::state::gBag.remove(content::ItemIDMoney, price)) {
            gWindow->closePopup();
            gWindow->runTalk(::hojy::content::gEvent.talk(0xB9F), 0x6F, 0);
            return;
        }
        ::hojy::world::state::gBag.add(shopInfo->id[index], 1);
        if (shopInfo->total[index] < 1000) {
            --shopInfo->total[index];
        }
        gWindow->closePopup();
        gWindow->runTalk(::hojy::content::gEvent.talk(0xBA0), 0x6F, 0);
    }, [this]() {
        subMap_->continueEvents(false);
        return false;
    });
    return true;
}

void Window::popupMessageBox(const std::vector<std::wstring> &text, MessageBox::Type type) {
    if (processingStage_) {
        defer([this, text, type] { popupMessageBox(text, type); });
        return;
    }
    MessageBox *messageBox;
    if (popup_) {
        messageBox = new MessageBox(popup_, 0, 0, width_, height_ * 4 / 5);
    } else {
        messageBox = new MessageBox(renderer_, 0, 0, width_, height_ * 4 / 5);
        popup_ = messageBox;
        freeOnClose_ = true;
    }
    messageBox->popup(text, type);
}

}
