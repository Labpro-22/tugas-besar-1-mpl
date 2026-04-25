#pragma once

#include <string>
#include <vector>

class Player;
class PropertyTile;
class StreetTile;
class LogEntry;

namespace CliOutputFormatter {
    std::vector<std::string> formatLogEntries(const std::vector<LogEntry>& entries);
    std::vector<std::string> formatMainMenu();
    std::vector<std::string> formatLoadCommandPrompt();
    std::vector<std::string> formatHelpCommands(const Player& player);
    std::vector<std::string> formatSectionTitle(const std::string& title);
    std::vector<std::string> formatTurnSummary(const Player& player);
    std::vector<std::string> formatDiceLanding(
        int die1,
        int die2,
        int total,
        const std::string& playerName,
        const std::string& tileName,
        const std::string& tileCode
    );
    std::vector<std::string> formatWinnerSummary(
        const std::vector<Player*>& winners,
        const std::vector<Player>& players,
        bool maxTurnReached
    );
    std::vector<std::string> formatStreetPurchasePreview(const PropertyTile& tile, const StreetTile& street);
    std::vector<std::string> formatPropertyDeed(const PropertyTile* property);
    std::vector<std::string> formatPlayerProperties(const Player& player);
}
