#pragma once

#include "renderer.hh"

#include <vector>
#include <cstdint>

namespace hojy::scene {

class Node {
    friend class Window;
public:
    Node(Node *parent, std::uint32_t width, std::uint32_t height): parent_(parent), renderer_(parent->renderer_), width_(width), height_(height){}
    Node(Renderer *renderer, std::uint32_t width, std::uint32_t height): parent_(nullptr), renderer_(renderer), width_(width), height_(height) {}
    Node(const Node&) = delete;
    void add(Node *child);
    void remove(Node *child);

    virtual void render() = 0;

protected:
    void doRender();

protected:
    Node *parent_;
    Renderer *renderer_;

    std::uint32_t width_, height_;
    bool visible_ = true;

    std::vector<Node*> children_;
};

}
