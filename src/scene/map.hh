#pragma once

#include "node.hh"

#include <cstdint>

namespace hojy::scene {

class Map final: public Node {
public:
    explicit Map(Renderer *renderer);
    Map(const Map&) = delete;

    void render() override;

private:
    std::int32_t width_ = 0, height_ = 0, cellWidth_ = 0, cellHeight_ = 0;
    std::vector<std::uint16_t> earth_, surface_, building_, buildx_, buildy_;
};

}
