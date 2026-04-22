#include "core/Board.hpp"
#include "models/tiles/Tile.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "models/tiles/RailroadTile.hpp"
#include "models/Player.hpp"
#include "models/Enums.hpp"
#include "models/tiles/UtilityTile.hpp"

Board::Board() = default;

Board::~Board() {
    for (Tile* tile : tiles) {
        delete tile;
    }
}

void Board::buildBoard(const std::vector<Tile*>& boardTiles) {
    for (Tile* tile : tiles) {
        delete tile;
    }
    tiles = boardTiles;
    propertyIndex.clear();
    for (Tile* tile : tiles) {
        if (tile->getCategory() == TileCategory::PROPERTY) {
            PropertyTile* prop = dynamic_cast<PropertyTile*>(tile);
            if (prop) {
                propertyIndex[prop->getCode()] = prop;
            }
        }
    }
}

Tile* Board::getTile(int index) const {
    if (index >= 0 && index < tiles.size()) {
        return tiles[index];
    }
    return nullptr;
}

Tile* Board::getTile(const std::string& code) const {
    for (Tile* tile : tiles) {
        if (tile && tile->getCode() == code) {
            return tile;
        }
    }
    return nullptr;
}

std::vector<PropertyTile*> Board::getProperties() const {
    std::vector<PropertyTile*> props;
    for (auto const& pair : propertyIndex) {
        props.push_back(pair.second);
    }
    return props;
}

std::vector<StreetTile*> Board::getProperties(ColorGroup colorGroup) const {
    std::vector<StreetTile*> streets;
    for (const auto& pair : propertyIndex) {
        StreetTile* street = dynamic_cast<StreetTile*>(pair.second);
        if (street && street->getColorGroup() == colorGroup) {
            streets.push_back(street);
        }
    }
    return streets;
}

RailroadTile* Board::getNearestRailroad(int fromIndex) const {
    if (tiles.empty()) return nullptr;
    int n = tiles.size();
    for (int i = 1; i <= n; ++i) {
        int idx = (fromIndex + i) % n;
        Tile* tile = tiles[idx];
        if (tile && tile->getCategory() == TileCategory::PROPERTY) {
            RailroadTile* rr = dynamic_cast<RailroadTile*>(tile);
            if (rr) {
                return rr;
            }
        }
    }
    return nullptr;
}

int Board::getTileCount() const {
    return tiles.size();
}

void Board::tickFestivals(Player& player) {
    for (auto const& pair : propertyIndex) {
        StreetTile* street = dynamic_cast<StreetTile*>(pair.second);
        if (street && street->isOwnedBy(player)) {
            street->tickFestival();
        }
    }
}

bool Board::hasMonopoly(const Player& player, ColorGroup colorGroup) const {
    std::vector<StreetTile*> streets = getProperties(colorGroup);
    if (streets.empty()) return false;
    
    for (StreetTile* street : streets) {
        if (!street->isOwnedBy(player)) {
            return false;
        }
    }
    return true;
}

int Board::getRailroadCount(const Player& player) const {
    int count = 0;
    for (auto const& pair : propertyIndex) {
        RailroadTile* rr = dynamic_cast<RailroadTile*>(pair.second);
        if (rr && rr->isOwnedBy(player)) {
            count++;
        }
    }
    return count;
}

int Board::getUtilityCount(const Player& player) const {
    int count = 0;
    for (auto const& pair : propertyIndex) {
        UtilityTile* util = dynamic_cast<UtilityTile*>(pair.second);
        if (util && util->isOwnedBy(player)) {
            count++;
        }
    }
    return count;
}
