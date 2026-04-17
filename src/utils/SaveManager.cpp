#include "utils/SaveManager.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "models/state/GameState.hpp"
#include "models/state/LogEntry.hpp"
#include "models/state/PlayerState.hpp"
#include "models/state/PropertyState.hpp"

std::string SaveManager::serializePlayer(const PlayerState& ps) const {
	std::string statusStr = "ACTIVE";
	if (ps.getStatus() == PlayerStatus::JAILED) {
		statusStr = "JAILED";
	} else if (ps.getStatus() == PlayerStatus::BANKRUPT) {
		statusStr = "BANKRUPT";
	}

	std::ostringstream oss;
	oss << ps.getUsername() << "|"
		<< ps.getBalance() << "|"
		<< ps.getPosition() << "|"
		<< statusStr << "|"
		<< ps.getJailTurns() << "|";

	const std::vector<std::string>& hand = ps.getCardHand();
	for (size_t i = 0; i < hand.size(); ++i) {
		if (i > 0) {
			oss << ",";
		}
		oss << hand[i];
	}

	return oss.str();
}

std::string SaveManager::serializeProperty(const PropertyState& ps) const {
	std::string propertyType = "UTILITY";
	if (ps.getPropertyType() == PropertyType::STREET) {
		propertyType = "STREET";
	} else if (ps.getPropertyType() == PropertyType::RAILROAD) {
		propertyType = "RAILROAD";
	}

	std::string propertyStatus = "BANK";
	if (ps.getStatus() == PropertyStatus::OWNED) {
		propertyStatus = "OWNED";
	} else if (ps.getStatus() == PropertyStatus::MORTGAGED) {
		propertyStatus = "MORTGAGED";
	}

	std::ostringstream oss;
	oss << ps.getCode() << "|"
		<< propertyType << "|"
		<< ps.getOwnerUsername() << "|"
		<< propertyStatus << "|"
		<< ps.getFestivalMultiplier() << "|"
		<< ps.getFestivalDuration() << "|"
		<< ps.getBuildingLevel();

	return oss.str();
}

std::string SaveManager::serializeDeck(const std::vector<std::string>& deck) const {
	std::ostringstream oss;

	for (size_t i = 0; i < deck.size(); ++i) {
		if (i > 0) {
			oss << ",";
		}
		oss << deck[i];
	}

	return oss.str();
}

std::string SaveManager::serializeLog(const std::vector<LogEntry>& entries) const {
	std::ostringstream oss;

	for (size_t i = 0; i < entries.size(); ++i) {
		if (i > 0) {
			oss << "\n";
		}

		oss << entries[i].getTurn() << "|"
			<< entries[i].getUsername() << "|"
			<< entries[i].getActionType() << "|"
			<< entries[i].getDetail();
	}

	return oss.str();
}

PlayerState SaveManager::parsePlayer(const std::string& line) const {
	std::vector<std::string> parts;
	std::istringstream ss(line);
	std::string token;
	while (std::getline(ss, token, '|')) {
		parts.push_back(token);
	}

	if (parts.size() != 6) {
		throw std::runtime_error("SaveManager: invalid PLAYER line -> '" + line + "'");
	}

	PlayerStatus status = PlayerStatus::ACTIVE;
	if (parts[3] == "JAILED") {
		status = PlayerStatus::JAILED;
	} else if (parts[3] == "BANKRUPT") {
		status = PlayerStatus::BANKRUPT;
	} else if (parts[3] != "ACTIVE") {
		throw std::runtime_error("SaveManager: invalid PlayerStatus '" + parts[3] + "'");
	}

	int balance = 0;
	int position = 0;
	int jailTurns = 0;
	try {
		balance = std::stoi(parts[1]);
		position = std::stoi(parts[2]);
		jailTurns = std::stoi(parts[4]);
	} catch (const std::exception&) {
		throw std::runtime_error("SaveManager: invalid numeric PLAYER line -> '" + line + "'");
	}

	std::vector<std::string> cardHand;
	if (!parts[5].empty()) {
		std::istringstream cardSS(parts[5]);
		std::string card;
		while (std::getline(cardSS, card, ',')) {
			cardHand.push_back(card);
		}
	}

	return PlayerState(parts[0], balance, position, status, jailTurns, cardHand);
}

PropertyState SaveManager::parseProperty(const std::string& line) const {
	std::vector<std::string> parts;
	std::istringstream ss(line);
	std::string token;
	while (std::getline(ss, token, '|')) {
		parts.push_back(token);
	}

	if (parts.size() != 7) {
		throw std::runtime_error("SaveManager: invalid PROPERTY line -> '" + line + "'");
	}

	PropertyType propertyType = PropertyType::UTILITY;
	if (parts[1] == "STREET") {
		propertyType = PropertyType::STREET;
	} else if (parts[1] == "RAILROAD") {
		propertyType = PropertyType::RAILROAD;
	} else if (parts[1] != "UTILITY") {
		throw std::runtime_error("SaveManager: invalid PropertyType '" + parts[1] + "'");
	}

	PropertyStatus propertyStatus = PropertyStatus::BANK;
	if (parts[3] == "OWNED") {
		propertyStatus = PropertyStatus::OWNED;
	} else if (parts[3] == "MORTGAGED") {
		propertyStatus = PropertyStatus::MORTGAGED;
	} else if (parts[3] != "BANK") {
		throw std::runtime_error("SaveManager: invalid PropertyStatus '" + parts[3] + "'");
	}

	int festivalMultiplier = 0;
	int festivalDuration = 0;
	try {
		festivalMultiplier = std::stoi(parts[4]);
		festivalDuration = std::stoi(parts[5]);
	} catch (const std::exception&) {
		throw std::runtime_error("SaveManager: invalid numeric PROPERTY line -> '" + line + "'");
	}

	return PropertyState(
		parts[0],
		propertyType,
		parts[2],
		propertyStatus,
		festivalMultiplier,
		festivalDuration,
		parts[6]
	);
}

void SaveManager::saveGame(const std::string& filename, const GameState& gameState) const {
	std::ofstream file(filename);
	if (!file.is_open()) {
		throw std::runtime_error("SaveManager: cannot open '" + filename + "' for writing");
	}

	file << "CURRENT_TURN|" << gameState.getCurrentTurn() << "\n";
	file << "MAX_TURN|" << gameState.getMaxTurn() << "\n";
	file << "ACTIVE_PLAYER|" << gameState.getActivePlayerUsername() << "\n";
	file << "TURN_ORDER|";

	const std::vector<std::string>& turnOrder = gameState.getTurnOrder();
	for (size_t i = 0; i < turnOrder.size(); ++i) {
		if (i > 0) {
			file << ",";
		}
		file << turnOrder[i];
	}
	file << "\n";

	const std::vector<PlayerState>& players = gameState.getPlayerStates();
	file << "PLAYERS|" << players.size() << "\n";
	for (const PlayerState& ps : players) {
		file << serializePlayer(ps) << "\n";
	}

	const std::vector<PropertyState>& properties = gameState.getPropertyStates();
	file << "PROPERTIES|" << properties.size() << "\n";
	for (const PropertyState& ps : properties) {
		file << serializeProperty(ps) << "\n";
	}

	file << "DECK|" << serializeDeck(gameState.getDeckState()) << "\n";

	const std::vector<LogEntry>& logs = gameState.getLogEntries();
	file << "LOGS|" << logs.size() << "\n";

	const std::string logBlock = serializeLog(logs);
	if (!logBlock.empty()) {
		file << logBlock << "\n";
	}
}

GameState SaveManager::loadGame(const std::string& filename) const {
	std::ifstream file(filename);
	if (!file.is_open()) {
		throw std::runtime_error("SaveManager: cannot open '" + filename + "' for reading");
	}

	auto readLineOrThrow = [&](const std::string& section) {
		std::string line;
		if (!std::getline(file, line)) {
			throw std::runtime_error("SaveManager: unexpected end while reading " + section);
		}
		return line;
	};

	auto parseSection = [&](const std::string& expectedSection) {
		std::string line = readLineOrThrow(expectedSection);
		size_t pos = line.find('|');
		if (pos == std::string::npos) {
			throw std::runtime_error("SaveManager: malformed line -> '" + line + "'");
		}

		std::string section = line.substr(0, pos);
		if (section != expectedSection) {
			throw std::runtime_error("SaveManager: expected " + expectedSection + ", got " + section);
		}

		return line.substr(pos + 1);
	};

	auto parseIntStrict = [&](const std::string& value, const std::string& fieldName) {
		try {
			size_t parsed = 0;
			int number = std::stoi(value, &parsed);
			if (parsed != value.size()) {
				throw std::runtime_error("invalid suffix");
			}
			return number;
		} catch (const std::exception&) {
			throw std::runtime_error("SaveManager: invalid integer for " + fieldName + " -> '" + value + "'");
		}
	};

	int currentTurn = parseIntStrict(parseSection("CURRENT_TURN"), "currentTurn");
	int maxTurn = parseIntStrict(parseSection("MAX_TURN"), "maxTurn");
	std::string activePlayerUsername = parseSection("ACTIVE_PLAYER");

	std::vector<std::string> turnOrder;
	std::string turnOrderLine = parseSection("TURN_ORDER");
	if (!turnOrderLine.empty()) {
		std::istringstream turnOrderStream(turnOrderLine);
		std::string username;
		while (std::getline(turnOrderStream, username, ',')) {
			turnOrder.push_back(username);
		}
	}

	int playerCount = parseIntStrict(parseSection("PLAYERS"), "playerCount");
	if (playerCount < 0) {
		throw std::runtime_error("SaveManager: negative playerCount");
	}

	std::vector<PlayerState> players;
	players.reserve(static_cast<size_t>(playerCount));
	for (int i = 0; i < playerCount; ++i) {
		players.push_back(parsePlayer(readLineOrThrow("player line")));
	}

	int propertyCount = parseIntStrict(parseSection("PROPERTIES"), "propertyCount");
	if (propertyCount < 0) {
		throw std::runtime_error("SaveManager: negative propertyCount");
	}

	std::vector<PropertyState> properties;
	properties.reserve(static_cast<size_t>(propertyCount));
	for (int i = 0; i < propertyCount; ++i) {
		properties.push_back(parseProperty(readLineOrThrow("property line")));
	}

	std::vector<std::string> deckState;
	std::string deckLine = parseSection("DECK");
	if (!deckLine.empty()) {
		std::istringstream deckStream(deckLine);
		std::string card;
		while (std::getline(deckStream, card, ',')) {
			deckState.push_back(card);
		}
	}

	int logCount = parseIntStrict(parseSection("LOGS"), "logCount");
	if (logCount < 0) {
		throw std::runtime_error("SaveManager: negative logCount");
	}

	std::vector<LogEntry> logs;
	logs.reserve(static_cast<size_t>(logCount));
	for (int i = 0; i < logCount; ++i) {
		std::vector<std::string> parts;
		std::istringstream logLineStream(readLineOrThrow("log line"));
		std::string part;
		while (std::getline(logLineStream, part, '|')) {
			parts.push_back(part);
		}

		if (parts.size() < 4) {
			throw std::runtime_error("SaveManager: invalid log line");
		}

		int turn = parseIntStrict(parts[0], "log.turn");
		std::string detail = parts[3];
		for (size_t j = 4; j < parts.size(); ++j) {
			detail += "|" + parts[j];
		}

		logs.emplace_back(turn, parts[1], parts[2], detail);
	}

	return GameState(
		currentTurn,
		maxTurn,
		players,
		turnOrder,
		activePlayerUsername,
		properties,
		deckState,
		logs
	);
}

bool SaveManager::fileExists(const std::string& filename) const {
	std::ifstream file(filename);
	return file.good();
}
