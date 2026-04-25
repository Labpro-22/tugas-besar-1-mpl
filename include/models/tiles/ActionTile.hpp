#pragma once

#include "Tile.hpp"

class ActionTile : public Tile {
public:
    ActionTile();
    ActionTile(int index, const std::string& code, const std::string& name);
    virtual ~ActionTile() = default;

    virtual void onLanded(Player& player, GameContext& gameContext) override = 0;
};
