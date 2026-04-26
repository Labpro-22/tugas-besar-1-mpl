#pragma once

#include <string>

class GameContext;
class Player;
class PropertyTile;

class Tile {
private:
    int index;
    std::string code;
    std::string name;

public:
    Tile();
    Tile(int index, const std::string& code, const std::string& name);
    virtual ~Tile() = default;

    int getIndex() const;
    const std::string& getCode() const;
    const std::string& getName() const;

    virtual void onLanded(Player& player, GameContext& gameContext) = 0;
    virtual void onPassed(Player& player, GameContext& gameContext);
    virtual void applyJailStatus(Player& player) const;
    virtual PropertyTile* asPropertyTile();
    virtual const PropertyTile* asPropertyTile() const;
    virtual int getJailFine() const;
};
