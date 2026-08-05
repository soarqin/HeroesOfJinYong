#include "title.hh"

namespace hojy::scene {

void Title::consumeKeyIntent(Node::Key key) {
    pendingInput_ = key;
}

void Title::consumeTextIntent(const std::wstring &str) {
    pendingInput_ = str;
}

}
