#include "window.hh"

namespace hojy::scene {

void Window::updateInput() {
    while (!sampledInputEvents_.empty() && !quitRequested_) {
        auto event = std::move(sampledInputEvents_.front());
        sampledInputEvents_.pop_front();
        if (event.action == core::InputAction::Quit) {
            quitRequested_ = true;
            sampledInputEvents_.clear();
            pendingInputEvents_.clear();
            break;
        }

        // Title screens and popup menus retain the original immediate input
        // behavior. Map input is staged for the next fixed-logic update.
        auto *target = popup_;
        if (!target) {
            pendingInputEvents_.push_back(std::move(event));
            continue;
        }
        auto intent = makeIntent(event);
        if (!intent) { continue; }

        const bool wasProcessing = processingStage_;
        processingStage_ = true;
        try {
            QueuedInputPort immediateInput;
            immediateInput.enqueue(std::move(intent));
            immediateInput.deliverNext(*target);
            target->dispatchInputLogic();

            processingStage_ = wasProcessing;
            if (!wasProcessing) {
                applyDeferredNodes();
                applyDeferredCommands();
            }
        } catch (...) {
            processingStage_ = wasProcessing;
            throw;
        }
        processingStage_ = wasProcessing;
    }
}

}
