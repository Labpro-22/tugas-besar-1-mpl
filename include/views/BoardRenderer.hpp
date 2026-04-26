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
    std::map<ColorGroup, std::string> colorMap;

public:
    void render(const Board& board, const std::vector<Player>& players, const TurnManager& turnManager);
    void renderLegend(const Board& board, const std::vector<Player>& players) const;
    std::string colorize(const std::string& text, const std::string& ansiCode) const;
};
