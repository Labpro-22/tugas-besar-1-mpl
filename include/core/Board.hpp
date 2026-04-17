#pragma once

#include <vector>
#include <map>
#include <string>

class Tile;
class PropertyTile;
class RailroadTile;
class StreetTile;
class Player;
enum class ColorGroup;

class Board {
private:
    std::vector<Tile*> tiles;
    std::map<std::string, PropertyTile*> propertyIndex;

public:
    Board();
    ~Board();

    void buildBoard(const std::vector<Tile*>& boardTiles);
    Tile* getTile(int index) const;
    Tile* getTile(const std::string& code) const;

    std::vector<PropertyTile*> getProperties() const;
    std::vector<StreetTile*> getProperties(ColorGroup colorGroup) const;

    RailroadTile* getNearestRailroad(int fromIndex) const;
    int getTileCount() const;
    void tickFestivals(Player& player);
};
