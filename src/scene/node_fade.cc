#include "node.hh"

#include "mask.hh"

namespace hojy::scene {

void Node::fadeIn(const std::function<void()> &postAction) {
    if (fadeNode_) {
        fadeNode_->requestDelete();
        fadeNode_ = nullptr;
    }
    fadePostAction_ = postAction;
    fadeNode_ = new Mask(this, Mask::FadeIn, 3);
}

void Node::fadeOut(const std::function<void()> &postAction) {
    if (fadeNode_) {
        fadeNode_->requestDelete();
        fadeNode_ = nullptr;
    }
    fadePostAction_ = postAction;
    fadeNode_ = new Mask(this, Mask::FadeOut, 3);
}

}
