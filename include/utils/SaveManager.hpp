#pragma once

#include <string>

class GameState;
class PlayerState;
class PropertyState;
class LogEntry;


class SaveManager {
private:
    std::string filePath;
    std::string serializePlayer(const PlayerState& ps) const;
    std::string serializeProperty(const PropertyState& ps) const;
    std::string serializeDeck(const std::vector<std::string>& deck) const;
    std::string serializeLog(const std::vector<LogEntry>& entries) const;

    PlayerState parsePlayer(const std::string& line) const; 
    PropertyState parseProperty(const std::string& line) const;

public:
    SaveManager() = default;
    void saveGame(const std::string& filename, const GameState& gameState) const;
    GameState loadGame(const std::string& filename) const;
    bool fileExists(const std::string& filename) const;
};