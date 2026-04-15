#pragma once
#include "Tile.hpp"

class ActionTile: public Tile{
private:

public:
    ActionTile();
    ActionTile(int index, string code, string name, TileCategory TileCategory);
    virtual ~ActionTile();

    virtual void onLanded(Player& player, GameContext& gameContext);
};