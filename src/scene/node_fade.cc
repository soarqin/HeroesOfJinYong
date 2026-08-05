#include "node.hh"

#include "mask.hh"

namespace hojy::scene {

void Node::fadeIn(const std::function<void()> &postAction) {
    // A persistent map can become inactive before its completed fade is
    // removed during presentation preparation. Starting another fade must
    // cancel that stale cleanup so it cannot delete the new mask.
    fadeCleanupRequested_ = false;
    runFadePostAction_ = false;
    if (fadeNode_) {
        fadeNode_->requestDelete();
        fadeNode_ = nullptr;
    }
    fadePostAction_ = postAction;
    fadeNode_ = new Mask(this, Mask::FadeIn, 3);
}

void Node::fadeOut(const std::function<void()> &postAction) {
    fadeCleanupRequested_ = false;
    runFadePostAction_ = false;
    if (fadeNode_) {
        fadeNode_->requestDelete();
        fadeNode_ = nullptr;
    }
    fadePostAction_ = postAction;
    fadeNode_ = new Mask(this, Mask::FadeOut, 3);
}

}
