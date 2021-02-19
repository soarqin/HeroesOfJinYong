#pragma once

#include "node.hh"

#include <cstdint>

namespace hojy::scene {

class Texture;

class Map final: public Node {
public:
    explicit Map(Renderer *renderer, std::uint32_t width, std::uint32_t height);
    Map(const Map&) = delete;
    ~Map();

    void render() override;

private:
    std::int32_t mapWidth_ = 0, mapHeight_ = 0, cellWidth_ = 0, cellHeight_ = 0;
    std::int32_t texWidth_ = 0, texHeight_ = 0, texWCount_ = 0, texHCount_ = 0;
    std::vector<std::uint16_t> earth_, surface_, building_, buildx_, buildy_;
    std::vector<Texture*> textures_;
};

}
