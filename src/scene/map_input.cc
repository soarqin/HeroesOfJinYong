#include "mapwithevent.hh"

namespace hojy::scene {

void MapWithEvent::consumeKeyIntent(Node::Key key) {
    pendingInputKey_ = key;
}

}
