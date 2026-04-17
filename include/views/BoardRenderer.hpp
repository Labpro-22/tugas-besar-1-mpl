#pragma once

#include <map>
#include <string>
#include <vector>

#include "core/Board.hpp"
#include "core/TurnManager.hpp"
#include "models/Enums.hpp"
#include "models/Player.hpp"
#include "models/tiles/Tile.hpp"

class BoardRenderer {
private:
    std::map<TileCategory, std::string> colorMap;

public:
    void render(Board& board, std::vector<Player>& players, TurnManager& turnManager);
    std::string renderTileCell(Tile* tile, std::vector<Player>& players);
    void renderLegend(std::vector<Player>& players);
    std::string colorize(const std::string& text, const std::string& ansiCode);
};
