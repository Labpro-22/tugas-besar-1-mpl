#include "models/tiles/Tile.hpp"

#include "models/Player.hpp"

Tile::Tile() : index(0), code(""), name("") {}

Tile::Tile(int index, const std::string& code, const std::string& name) :
    index(index), code(code), name(name) {}

int Tile::getIndex() const {
    return index;
}

const std::string& Tile::getCode() const {
    return code;
}

const std::string& Tile::getName() const {
    return name;
}

void Tile::onPassed(Player&, GameContext&) {}

void Tile::applyJailStatus(Player& player) const {
    player.setPosition(getIndex());
    player.setStatus(PlayerStatus::JAILED);
    player.setJailTurns(0);
    player.setConsecutiveDoubles(0);
}

PropertyTile* Tile::asPropertyTile() {
    return nullptr;
}

const PropertyTile* Tile::asPropertyTile() const {
    return nullptr;
}

int Tile::getJailFine() const {
    return 0;
}
