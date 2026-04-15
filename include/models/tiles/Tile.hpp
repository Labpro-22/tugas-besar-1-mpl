#pragma once
#include <string>
#include "../Enums.hpp"
using namespace std;

class GameContext;
class Player;

class Tile{
private:
    int index;
    string code;
    string name;
    TileCategory category;

public:
    Tile();
    Tile(int index, string code, string name, TileCategory TileCategory);
    virtual ~Tile() = default;

    int getIndex() const;
    string getCode() const;
    string getName() const;
    TileCategory getCategory() const;

    virtual void onLanded(Player& player, GameContext& gameContext) = 0;
    virtual string getDisplayLabel() const = 0;
};
