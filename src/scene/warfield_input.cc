#include "warfield.hh"

namespace hojy::scene {

void Warfield::consumeKeyIntent(Node::Key key) {
    pendingModeKey_ = key;
    hasPendingModeKey_ = true;
}

}
