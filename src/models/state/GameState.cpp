#include "models/state/GameState.hpp"

GameState::GameState()
	: currentTurn(1),
	  maxTurn(0),
	  playerStates(),
	  turnOrder(),
	  activePlayerUsername(""),
	  propertyStates(),
	  deckState(),
	  logEntries(),
	  configPath("") {}

GameState::GameState(
	int currentTurn,
	int maxTurn,
	const std::vector<PlayerState>& playerStates,
	const std::vector<std::string>& turnOrder,
	const std::string& activePlayerUsername,
	const std::vector<PropertyState>& propertyStates,
	const std::vector<std::string>& deckState,
	const std::vector<LogEntry>& logEntries,
	const std::string& configPath
) : currentTurn(currentTurn),
	maxTurn(maxTurn),
	playerStates(playerStates),
	turnOrder(turnOrder),
	activePlayerUsername(activePlayerUsername),
	propertyStates(propertyStates),
	deckState(deckState),
	logEntries(logEntries),
	configPath(configPath) {}

int GameState::getCurrentTurn() const {
	return currentTurn;
}

int GameState::getMaxTurn() const {
	return maxTurn;
}

std::vector<PlayerState>& GameState::getPlayerStates() {
	return playerStates;
}

const std::vector<PlayerState>& GameState::getPlayerStates() const {
	return playerStates;
}

std::vector<std::string>& GameState::getTurnOrder() {
	return turnOrder;
}

const std::vector<std::string>& GameState::getTurnOrder() const {
	return turnOrder;
}

const std::string& GameState::getActivePlayerUsername() const {
	return activePlayerUsername;
}

std::vector<PropertyState>& GameState::getPropertyStates() {
	return propertyStates;
}

const std::vector<PropertyState>& GameState::getPropertyStates() const {
	return propertyStates;
}

std::vector<std::string>& GameState::getDeckState() {
	return deckState;
}

const std::vector<std::string>& GameState::getDeckState() const {
	return deckState;
}

std::vector<LogEntry>& GameState::getLogEntries() {
	return logEntries;
}

const std::vector<LogEntry>& GameState::getLogEntries() const {
	return logEntries;
}

const std::string& GameState::getConfigPath() const {
	return configPath;
}
