#pragma once

#include <string>
#include <vector>

#include "models/state/LogEntry.hpp"
#include "models/state/PlayerState.hpp"
#include "models/state/PropertyState.hpp"

class GameState {
private:
    int currentTurn;
    int maxTurn;
    std::vector<PlayerState> playerStates;
    std::vector<std::string> turnOrder;
    std::string activePlayerUsername;
    std::vector<PropertyState> propertyStates;
    std::vector<std::string> deckState;
    std::vector<LogEntry> logEntries;

public:
    GameState();
    GameState(
        int currentTurn,
        int maxTurn,
        const std::vector<PlayerState>& playerStates,
        const std::vector<std::string>& turnOrder,
        const std::string& activePlayerUsername,
        const std::vector<PropertyState>& propertyStates,
        const std::vector<std::string>& deckState,
        const std::vector<LogEntry>& logEntries
    );

    int getCurrentTurn() const;
    int getMaxTurn() const;
    std::vector<PlayerState>& getPlayerStates();
    const std::vector<PlayerState>& getPlayerStates() const;
    std::vector<std::string>& getTurnOrder();
    const std::vector<std::string>& getTurnOrder() const;
    const std::string& getActivePlayerUsername() const;
    std::vector<PropertyState>& getPropertyStates();
    const std::vector<PropertyState>& getPropertyStates() const;
    std::vector<std::string>& getDeckState();
    const std::vector<std::string>& getDeckState() const;
    std::vector<LogEntry>& getLogEntries();
    const std::vector<LogEntry>& getLogEntries() const;
};
