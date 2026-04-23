#pragma once

#include <string>

#include "models/Enums.hpp"

class GameContext;
class Player;
class PropertyTile;

class Tile {
private:
    int index;
    std::string code;
    std::string name;
    TileCategory category;

public:
    Tile();
    Tile(int index, const std::string& code, const std::string& name, TileCategory category);
    virtual ~Tile() = default;

    int getIndex() const;
    const std::string& getCode() const;
    const std::string& getName() const;
    TileCategory getCategory() const;

    virtual void onLanded(Player& player, GameContext& gameContext) = 0;
    virtual void onPassed(Player& player, GameContext& gameContext);
    virtual void applyJailStatus(Player& player) const;
    virtual PropertyTile* asPropertyTile();
    virtual const PropertyTile* asPropertyTile() const;
    virtual int getJailFine() const;
};
