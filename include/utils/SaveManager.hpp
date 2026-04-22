#pragma once

#include <istream>
#include <string>
#include <vector>

#include "models/Enums.hpp"

class GameState;
class PlayerState;
class PropertyState;
class LogEntry;

class SaveManager {
private:
    std::string filePath;

    std::string resolveDataPath(const std::string& filename) const;
    std::string statusToString(PlayerStatus status) const;
    std::string propertyTypeToString(PropertyType type) const;
    std::string propertyStatusToString(PropertyStatus status) const;
    std::string serializeCard(const std::string& encodedCard) const;

    std::string readLineOrThrow(std::istream& input, const std::string& section) const;
    int parseIntStrict(const std::string& value, const std::string& fieldName) const;
    std::vector<std::string> splitWhitespace(const std::string& value) const;
    std::string parseCard(const std::string& line) const;
    LogEntry parseLog(const std::string& line) const;
    PlayerState parsePlayer(std::istream& input) const;
    PropertyState parseProperty(const std::string& line) const;

public:
    SaveManager() = default;

    void saveGame(const std::string& filename, const GameState& gameState) const;
    GameState loadGame(const std::string& filename) const;
    bool fileExists(const std::string& filename) const;
};
