#include "models/tiles/ActionTile.hpp"

ActionTile::ActionTile() : Tile() {}

ActionTile::ActionTile(int index, const std::string& code, const std::string& name)
    : Tile(index, code, name) {}
