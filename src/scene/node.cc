#include "node.hh"

#include <algorithm>

namespace hojy::scene {

void Node::add(Node *child) {
    children_.push_back(child);
}

void Node::remove(Node *child) {
    children_.erase(std::remove(children_.begin(), children_.end(), child), children_.end());
}

void Node::doRender() {
    render();
    for (auto *node : children_) {
        node->doRender();
    }
}

}
