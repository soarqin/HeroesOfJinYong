/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "logic/title_input_mode.hh"
#include "logic/title_snapshot.hh"
#include "nodewithcache.hh"
#include "texture.hh"
#include "world/new_game_transaction.hh"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>

namespace hojy::scene {

class Title final: public NodeWithCache, public TitleInputExecutionContext {
public:
    using NodeWithCache::NodeWithCache;
    ~Title() override;

    [[nodiscard]] bool init();
    void setFontSize(int size) noexcept { fontSize_ = size > 0 ? size : 16; }

    [[nodiscard]] const TitleScreenSnapshot &screenSnapshot() const noexcept {
        return snapshot_;
    }

    void applyInputLogic() override;
    void consumeKeyIntent(Key key) override;
    void consumeTextIntent(const std::wstring &str) override;

    // TitleInputExecutionContext
    void executeMoveSelection(int delta) override;
    void executeActivateMainSelection() override;
    void executeActivateLoadSelection() override;
    void executeReturnToMainMenu() override;
    void executeAppendName(std::wstring text) override;
    void executeEraseName() override;
    void executeSubmitName() override;
    void executeSelectConfirmation(int index) override;
    void executeActivateConfirmation() override;
    void executeRerollCandidate() override;

private:
    bool prepareTextResources() override;
    void makeCache() override;

    void enterMainMenu();
    void enterLoadMenu();
    void enterNameEntry();
    void enterPreview();
    void showInputFailure();
    void startDefaultNewGame();
    void startNameEntry();
    void requestLoadMenu();
    void requestQuit();
    [[nodiscard]] bool prepareCandidate();
    [[nodiscard]] bool setCandidateIdentity();
    [[nodiscard]] bool rerollCandidate();
    void queueTextInputRect();
    void queueCandidateActivation();

private:
    TextureMgr titleTextureMgr_;
    Texture *big_ = nullptr;

    std::unique_ptr<TitleInputMode> inputMode_ =
        std::make_unique<TitleMainMenuInputMode>();
    TitleScreenSnapshot snapshot_ = TitleMainMenuSnapshot{};
    std::variant<std::monostate, Key, std::wstring> pendingInput_;

    int selection_ = 0;
    int confirmationIndex_ = -1;
    std::wstring mainCharName_;
    int fontSize_ = 16;
    std::optional<::hojy::world::state::NewGameCandidate> candidate_;
    std::uint64_t snapshotGeneration_ = 0;
};

}
