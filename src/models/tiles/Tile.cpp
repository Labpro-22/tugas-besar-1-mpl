#include "models/tiles/Tile.hpp"

Tile::Tile() : index(0), code(""), name(""), category(TileCategory::DEFAULT) {}

Tile::Tile(int index, const std::string& code, const std::string& name, TileCategory category) :
    index(index), code(code), name(name), category(category) {}

int Tile::getIndex() const {
    return index;
}

const std::string& Tile::getCode() const {
    return code;
}

const std::string& Tile::getName() const {
    return name;
}

TileCategory Tile::getCategory() const {
    return category;
}