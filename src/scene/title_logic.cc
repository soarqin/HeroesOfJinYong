#include "title.hh"

#include "content/factors.hh"
#include "core/config.hh"
#include "scene/title_snapshot_builder.hh"
#include "util/conv.hh"
#include "util/random.hh"
#include "window_command.hh"
#include "world/strings.hh"

#include <array>
#include <type_traits>
#include <utility>
#include <vector>

namespace hojy::scene {
namespace {

class GlobalNewGameRandom final: public ::hojy::world::state::NewGameRandom {
public:
    int next(int minimum, int maximum) override {
        return static_cast<int>(::hojy::util::gRandom(
            static_cast<::hojy::util::Random::IntType>(minimum),
            static_cast<::hojy::util::Random::IntType>(maximum)));
    }
};

class StartNewGameCommand final: public SceneCommand {
public:
    StartNewGameCommand(
            ::hojy::world::state::NewGameCandidate candidate,
            std::vector<std::wstring> failureMessage)
        : candidate_(std::move(candidate)),
          failureMessage_(std::move(failureMessage)) {}

    void execute(SceneCommandContext &context) override {
        const bool started = context.startNewGame(std::move(candidate_));
        if (started) {
            context.closePopup();
        } else {
            context.showMessage(
                std::move(failureMessage_), ScenePopupType::PressToCloseThis);
        }
    }

private:
    ::hojy::world::state::NewGameCandidate candidate_;
    std::vector<std::wstring> failureMessage_;
};

}

void Title::applyInputLogic() {
    if (std::holds_alternative<std::monostate>(pendingInput_)) { return; }

    auto pending = std::move(pendingInput_);
    pendingInput_ = std::monostate{};
    std::unique_ptr<TitleInputAction> action;
    std::visit([this, &action](auto &&value) {
        using Value = std::decay_t<decltype(value)>;
        if constexpr (std::is_same<Value, Key>::value) {
            action = inputMode_->keyAction(value);
        } else if constexpr (std::is_same<Value, std::wstring>::value) {
            action = inputMode_->textAction(std::move(value));
        }
    }, std::move(pending));
    if (action) { action->execute(*this); }
}

void Title::executeMoveSelection(int delta) {
    std::visit([this, delta](auto &snapshot) {
        using Snapshot = std::decay_t<decltype(snapshot)>;
        if constexpr (std::is_same<Snapshot, TitleMainMenuSnapshot>::value
                      || std::is_same<Snapshot, TitleLoadMenuSnapshot>::value) {
            constexpr int count = 3;
            auto selected = snapshot.selectedIndex + delta;
            if (selected < 0) { selected = count - 1; }
            if (selected >= count) { selected = 0; }
            snapshot.selectedIndex = selected;
            selection_ = selected;
            requestPresentationRefresh();
        }
    }, snapshot_);
}

void Title::executeActivateMainSelection() {
    const auto *main = std::get_if<TitleMainMenuSnapshot>(&snapshot_);
    if (!main || main->selectedIndex < 0 || main->selectedIndex >= 3) {
        return;
    }
    using Action = void (Title::*)();
    const std::array<Action, 3> actions = {
        core::config.noNameInput()
            ? &Title::startDefaultNewGame : &Title::startNameEntry,
        &Title::requestLoadMenu,
        &Title::requestQuit,
    };
    (this->*actions[static_cast<std::size_t>(main->selectedIndex)])();
}

void Title::executeActivateLoadSelection() {
    const auto *load = std::get_if<TitleLoadMenuSnapshot>(&snapshot_);
    if (!load || load->selectedIndex < 0 || load->selectedIndex >= 3) {
        return;
    }
    const int slot = load->selectedIndex + 1;
    postOwnedSceneCommand(this, [slot](Title &, SceneCommandContext &context) {
        if (context.loadGame(slot)) {
            context.closePopup();
        } else {
            context.showMessage({GETTEXT(69)}, ScenePopupType::PressToCloseThis);
        }
    });
}

void Title::executeReturnToMainMenu() {
    if (std::holds_alternative<TitleNameEntrySnapshot>(snapshot_)) {
        postCommand([](SceneCommandContext &context) { context.endTextInput(); });
    }
    enterMainMenu();
}

void Title::executeAppendName(std::wstring text) {
    for (const auto ch: text) {
        if (mainCharName_.size() >= 8 || ch == L' ') { continue; }
        mainCharName_ += ch;
    }
    std::get<TitleNameEntrySnapshot>(snapshot_) =
        buildTitleNameEntrySnapshot(mainCharName_);
    queueTextInputRect();
    requestPresentationRefresh();
}

void Title::executeEraseName() {
    if (mainCharName_.empty()) { return; }
    mainCharName_.pop_back();
    std::get<TitleNameEntrySnapshot>(snapshot_) =
        buildTitleNameEntrySnapshot(mainCharName_);
    queueTextInputRect();
    requestPresentationRefresh();
}

void Title::executeSubmitName() {
    postCommand([](SceneCommandContext &context) { context.endTextInput(); });
    if (!prepareCandidate() || !setCandidateIdentity()) {
        candidate_.reset();
        enterMainMenu();
        showInputFailure();
        return;
    }
    enterPreview();
}

void Title::executeSelectConfirmation(int index) {
    if (!std::holds_alternative<TitlePreviewSnapshot>(snapshot_)) { return; }
    confirmationIndex_ = index == 1 ? 1 : 0;
    auto &preview = std::get<TitlePreviewSnapshot>(snapshot_);
    preview.confirmationIndex = confirmationIndex_;
    ++snapshotGeneration_;
    preview.generation = snapshotGeneration_;
    requestPresentationRefresh();
}

void Title::executeActivateConfirmation() {
    if (!std::holds_alternative<TitlePreviewSnapshot>(snapshot_)) { return; }
    if (confirmationIndex_ == 0) {
        queueCandidateActivation();
    } else {
        executeRerollCandidate();
    }
}

void Title::executeRerollCandidate() {
    if (!rerollCandidate()) {
        showInputFailure();
        return;
    }
    confirmationIndex_ = 0;
    enterPreview();
}

void Title::enterMainMenu() {
    selection_ = 0;
    confirmationIndex_ = 0;
    inputMode_ = std::make_unique<TitleMainMenuInputMode>();
    snapshot_ = TitleMainMenuSnapshot{};
    requestPresentationRefresh();
}

void Title::enterLoadMenu() {
    selection_ = 0;
    inputMode_ = std::make_unique<TitleLoadInputMode>();
    snapshot_ = TitleLoadMenuSnapshot{};
    requestPresentationRefresh();
}

void Title::enterNameEntry() {
    inputMode_ = std::make_unique<TitleNameInputMode>();
    snapshot_ = buildTitleNameEntrySnapshot(mainCharName_);
    queueTextInputRect();
    requestPresentationRefresh();
}

void Title::enterPreview() {
    if (!candidate_ || !candidate_->saveData().charInfo[0]) {
        enterMainMenu();
        showInputFailure();
        return;
    }
    inputMode_ = std::make_unique<TitleConfirmationInputMode>();
    const auto &character = *candidate_->saveData().charInfo[0];
    ++snapshotGeneration_;
    snapshot_ = buildTitlePreviewSnapshot(
        mainCharName_, character, core::config.showPotential(),
        confirmationIndex_, snapshotGeneration_);
    requestPresentationRefresh();
}

void Title::showInputFailure() {
    postCommand([](SceneCommandContext &context) {
        context.showMessage({GETTEXT(69)}, ScenePopupType::PressToCloseThis);
    });
}

void Title::startDefaultNewGame() {
    mainCharName_ = core::config.defaultName();
    if (!prepareCandidate() || !setCandidateIdentity()) {
        candidate_.reset();
        mainCharName_.clear();
        enterMainMenu();
        showInputFailure();
        return;
    }
    enterPreview();
}

void Title::startNameEntry() {
    mainCharName_.clear();
    postCommand([](SceneCommandContext &context) { context.beginTextInput(); });
    enterNameEntry();
}

void Title::requestLoadMenu() {
    enterLoadMenu();
}

void Title::requestQuit() {
    postCommand([](SceneCommandContext &context) {
        context.closePopup();
        context.forceQuit();
    });
}

bool Title::prepareCandidate() {
    GlobalNewGameRandom random;
    auto candidate = ::hojy::world::state::prepareNewGameCandidate(
        random, ::hojy::content::gFactors.initSubMapId);
    if (!candidate) {
        candidate_.reset();
        return false;
    }
    candidate_ = std::move(*candidate);
    return true;
}

bool Title::setCandidateIdentity() {
    if (!candidate_) { return false; }
    auto encodedName = ::hojy::util::big5Conv.fromUnicode(mainCharName_);
    while (encodedName.size() > 8 && !mainCharName_.empty()) {
        mainCharName_.pop_back();
        encodedName = ::hojy::util::big5Conv.fromUnicode(mainCharName_);
    }
    const auto suffix = ::hojy::util::big5Conv.fromUnicode(GETTEXT(110));
    return candidate_->setIdentity(encodedName, suffix);
}

bool Title::rerollCandidate() {
    if (!candidate_) { return false; }
    GlobalNewGameRandom random;
    return candidate_->reroll(random);
}

void Title::queueTextInputRect() {
    const auto x = width_ / 4
        + static_cast<int>((GETTEXT(41).size() + mainCharName_.size())
                           * fontSize_ / 2);
    const auto y = height_ - fontSize_ * 5 - TextLineSpacing * 4;
    const auto w = width_ / 2;
    const auto h = fontSize_;
    postCommand([x, y, w, h](SceneCommandContext &context) {
        context.setTextInputRect(x, y, w, h);
    });
}

void Title::queueCandidateActivation() {
    if (!candidate_) { return; }
    auto candidate = std::move(*candidate_);
    candidate_.reset();
    postOwnedSceneCommand(
        this,
        std::make_unique<StartNewGameCommand>(
            std::move(candidate), std::vector<std::wstring>{GETTEXT(69)}));
}

}
