#pragma once

#include "node.hh"

#include <cstdint>

namespace hojy::scene {

class Texture;

class Map final: public Node {
    struct BuildingTex {
        std::int32_t order;
        std::int32_t x, y;
        const Texture *tex;
    };
    struct BuildingTexComp {
        bool operator()(const BuildingTex &a, const BuildingTex &b) const {
            return a.order < b.order;
        }
    };

public:
    enum Direction {
        DirUp = 0,
        DirRight = 1,
        DirLeft = 2,
        DirDown = 3,
    };

public:
    explicit Map(Renderer *renderer, std::uint32_t width, std::uint32_t height);
    Map(const Map&) = delete;
    ~Map();

    void setPosition(int x, int y);
    void move(Direction direction);

    void render() override;

private:
    std::int32_t currX_ = 0, currY_ = 0, currFrame_ = 0;
    Direction direction_ = DirUp;
    bool moveDirty_ = false;
    std::int32_t mapWidth_ = 0, mapHeight_ = 0, cellWidth_ = 0, cellHeight_ = 0;
    std::int32_t texWidth_ = 0, texHeight_ = 0, texWCount_ = 0, texHCount_ = 0;
    std::int32_t offsetX_ = 0, offsetY_ = 0;
    std::vector<std::uint16_t> earth_, surface_, building_, buildx_, buildy_;
    std::vector<Texture*> terrainTex_;
    std::vector<BuildingTex> buildingTex_;
    Texture *drawingBuildingTex_[2] = {nullptr, nullptr};
};

}
