#pragma once

#include "renderer.hh"

#include <vector>

namespace hojy::scene {

class Node {
    friend class Window;
public:
    explicit Node(Node *parent): parent_(parent), renderer_(parent->renderer_) {}
    explicit Node(Renderer *renderer): parent_(nullptr), renderer_(renderer) {}
    Node(const Node&) = delete;
    void add(Node *child);
    void remove(Node *child);

    virtual void render() = 0;

protected:
    void doRender();

protected:
    Renderer *renderer_;
    bool visible_ = true;

    Node *parent_;
    std::vector<Node*> children_;
};

}
