#include "utils/SaveManager.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "models/state/GameState.hpp"
#include "models/state/LogEntry.hpp"
#include "models/state/PlayerState.hpp"
#include "models/state/PropertyState.hpp"

std::string SaveManager::resolveDataPath(const std::string& filename) const {
    std::string normalized = filename;
    for (char& c : normalized) {
        if (c == '\\') {
            c = '/';
        }
    }

    const std::string dataPrefix = "data/";
    if (normalized.rfind(dataPrefix, 0) == 0) {
        if (normalized.size() < 4 || normalized.substr(normalized.size() - 4) != ".txt") {
            throw std::runtime_error("SaveManager: save file must use .txt extension");
        }
        return normalized;
    }

    if (normalized.size() < 4 || normalized.substr(normalized.size() - 4) != ".txt") {
        throw std::runtime_error("SaveManager: save file must use .txt extension");
    }
    return dataPrefix + normalized;
}

std::string SaveManager::statusToString(PlayerStatus status) const {
    if (status == PlayerStatus::JAILED) {
        return "JAILED";
    }
    if (status == PlayerStatus::BANKRUPT) {
        return "BANKRUPT";
    }
    return "ACTIVE";
}

std::string SaveManager::propertyTypeToString(PropertyType type) const {
    if (type == PropertyType::RAILROAD) {
        return "railroad";
    }
    if (type == PropertyType::UTILITY) {
        return "utility";
    }
    return "street";
}

std::string SaveManager::propertyStatusToString(PropertyStatus status) const {
    if (status == PropertyStatus::OWNED) {
        return "OWNED";
    }
    if (status == PropertyStatus::MORTGAGED) {
        return "MORTGAGED";
    }
    return "BANK";
}

std::string SaveManager::serializeCard(const std::string& encodedCard) const {
    std::size_t separator = encodedCard.find(':');
    if (separator == std::string::npos) {
        return encodedCard;
    }

    std::string type = encodedCard.substr(0, separator);
    std::string value = encodedCard.substr(separator + 1);
    if (type == "DiscountCard") {
        return type + " " + value + " 1";
    }
    return type + " " + value;
}

std::string SaveManager::readLineOrThrow(std::istream& input, const std::string& section) const {
    std::string line;
    if (!std::getline(input, line)) {
        throw std::runtime_error("SaveManager: unexpected end while reading " + section);
    }
    return line;
}

int SaveManager::parseIntStrict(const std::string& value, const std::string& fieldName) const {
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
}

std::vector<std::string> SaveManager::splitWhitespace(const std::string& value) const {
    std::vector<std::string> result;
    std::istringstream stream(value);
    std::string token;
    while (stream >> token) {
        result.push_back(token);
    }
    return result;
}

std::string SaveManager::parseCard(const std::string& line) const {
    std::vector<std::string> parts = splitWhitespace(line);
    if (parts.empty()) {
        throw std::runtime_error("SaveManager: invalid card line");
    }

    if (parts[0] == "MoveCard" || parts[0] == "DiscountCard") {
        if (parts.size() < 2) {
            throw std::runtime_error("SaveManager: missing card value for " + parts[0]);
        }
        parseIntStrict(parts[1], "card.value");
        return parts[0] + ":" + parts[1];
    }

    return parts[0];
}

LogEntry SaveManager::parseLog(const std::string& line) const {
    std::istringstream stream(line);
    std::string turnText;
    std::string username;
    std::string actionType;

    if (!(stream >> turnText >> username >> actionType)) {
        throw std::runtime_error("SaveManager: invalid log line");
    }

    std::string detail;
    std::getline(stream, detail);
    if (!detail.empty() && detail[0] == ' ') {
        detail.erase(0, 1);
    }

    return LogEntry(parseIntStrict(turnText, "log.turn"), username, actionType, detail);
}

PlayerState SaveManager::parsePlayer(std::istream& input) const {
    std::string header = readLineOrThrow(input, "player state");
    std::vector<std::string> parts = splitWhitespace(header);
    if (parts.size() != 4) {
        throw std::runtime_error("SaveManager: invalid PLAYER header -> '" + header + "'");
    }

    PlayerStatus status = PlayerStatus::ACTIVE;
    if (parts[3] == "JAILED") {
        status = PlayerStatus::JAILED;
    } else if (parts[3] == "BANKRUPT") {
        status = PlayerStatus::BANKRUPT;
    } else if (parts[3] != "ACTIVE") {
        throw std::runtime_error("SaveManager: invalid PlayerStatus '" + parts[3] + "'");
    }

    int cardCount = parseIntStrict(readLineOrThrow(input, "player card count"), "player.cardCount");
    if (cardCount < 0) {
        throw std::runtime_error("SaveManager: negative player card count");
    }

    std::vector<std::string> cardHand;
    cardHand.reserve(static_cast<size_t>(cardCount));
    for (int i = 0; i < cardCount; ++i) {
        cardHand.push_back(parseCard(readLineOrThrow(input, "player card")));
    }

    return PlayerState(
        parts[0],
        parseIntStrict(parts[1], "player.balance"),
        0,
        parts[2],
        status,
        0,
        cardHand
    );
}

PropertyState SaveManager::parseProperty(const std::string& line) const {
    std::vector<std::string> parts = splitWhitespace(line);
    if (parts.size() != 7) {
        throw std::runtime_error("SaveManager: invalid PROPERTY line -> '" + line + "'");
    }

    PropertyType propertyType = PropertyType::STREET;
    if (parts[1] == "railroad" || parts[1] == "RAILROAD") {
        propertyType = PropertyType::RAILROAD;
    } else if (parts[1] == "utility" || parts[1] == "UTILITY") {
        propertyType = PropertyType::UTILITY;
    } else if (parts[1] != "street" && parts[1] != "STREET") {
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

    return PropertyState(
        parts[0],
        propertyType,
        parts[2],
        propertyStatus,
        parseIntStrict(parts[4], "property.festivalMultiplier"),
        parseIntStrict(parts[5], "property.festivalDuration"),
        parts[6]
    );
}

void SaveManager::saveGame(const std::string& filename, const GameState& gameState) const {
    std::filesystem::create_directories("data");
    std::string path = resolveDataPath(filename);
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("SaveManager: cannot open '" + path + "' for writing");
    }

    file << gameState.getCurrentTurn() << " " << gameState.getMaxTurn() << "\n";

    const std::vector<PlayerState>& players = gameState.getPlayerStates();
    file << players.size() << "\n";
    for (const PlayerState& player : players) {
        file << player.getUsername() << " "
             << player.getBalance() << " "
             << player.getPositionCode() << " "
             << statusToString(player.getStatus()) << "\n";

        const std::vector<std::string>& hand = player.getCardHand();
        file << hand.size() << "\n";
        for (const std::string& card : hand) {
            file << serializeCard(card) << "\n";
        }
    }

    const std::vector<std::string>& turnOrder = gameState.getTurnOrder();
    for (size_t i = 0; i < turnOrder.size(); ++i) {
        if (i > 0) {
            file << " ";
        }
        file << turnOrder[i];
    }
    file << "\n";
    file << gameState.getActivePlayerUsername() << "\n";

    const std::vector<PropertyState>& properties = gameState.getPropertyStates();
    file << properties.size() << "\n";
    for (const PropertyState& property : properties) {
        file << property.getCode() << " "
             << propertyTypeToString(property.getPropertyType()) << " "
             << property.getOwnerUsername() << " "
             << propertyStatusToString(property.getStatus()) << " "
             << property.getFestivalMultiplier() << " "
             << property.getFestivalDuration() << " "
             << property.getBuildingLevel() << "\n";
    }

    const std::vector<std::string>& deckState = gameState.getDeckState();
    file << deckState.size() << "\n";
    for (const std::string& card : deckState) {
        file << card << "\n";
    }

    const std::vector<LogEntry>& logs = gameState.getLogEntries();
    file << logs.size() << "\n";
    for (const LogEntry& log : logs) {
        file << log.getTurn() << " "
             << log.getUsername() << " "
             << log.getActionType() << " "
             << log.getDetail() << "\n";
    }
}

GameState SaveManager::loadGame(const std::string& filename) const {
    std::string path = resolveDataPath(filename);
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("SaveManager: cannot open '" + path + "' for reading");
    }

    std::vector<std::string> turnParts = splitWhitespace(readLineOrThrow(file, "turn header"));
    if (turnParts.size() != 2) {
        throw std::runtime_error("SaveManager: invalid turn header");
    }
    int currentTurn = parseIntStrict(turnParts[0], "currentTurn");
    int maxTurn = parseIntStrict(turnParts[1], "maxTurn");

    int playerCount = parseIntStrict(readLineOrThrow(file, "player count"), "playerCount");
    if (playerCount < 0) {
        throw std::runtime_error("SaveManager: negative playerCount");
    }

    std::vector<PlayerState> players;
    players.reserve(static_cast<size_t>(playerCount));
    for (int i = 0; i < playerCount; ++i) {
        players.push_back(parsePlayer(file));
    }

    std::vector<std::string> turnOrder = splitWhitespace(readLineOrThrow(file, "turn order"));
    std::string activePlayerUsername = readLineOrThrow(file, "active player");

    int propertyCount = parseIntStrict(readLineOrThrow(file, "property count"), "propertyCount");
    if (propertyCount < 0) {
        throw std::runtime_error("SaveManager: negative propertyCount");
    }

    std::vector<PropertyState> properties;
    properties.reserve(static_cast<size_t>(propertyCount));
    for (int i = 0; i < propertyCount; ++i) {
        properties.push_back(parseProperty(readLineOrThrow(file, "property line")));
    }

    int deckCount = parseIntStrict(readLineOrThrow(file, "deck count"), "deckCount");
    if (deckCount < 0) {
        throw std::runtime_error("SaveManager: negative deckCount");
    }

    std::vector<std::string> deckState;
    deckState.reserve(static_cast<size_t>(deckCount));
    for (int i = 0; i < deckCount; ++i) {
        std::vector<std::string> parts = splitWhitespace(readLineOrThrow(file, "deck card"));
        if (!parts.empty()) {
            deckState.push_back(parts[0]);
        }
    }

    int logCount = parseIntStrict(readLineOrThrow(file, "log count"), "logCount");
    if (logCount < 0) {
        throw std::runtime_error("SaveManager: negative logCount");
    }

    std::vector<LogEntry> logs;
    logs.reserve(static_cast<size_t>(logCount));
    for (int i = 0; i < logCount; ++i) {
        logs.push_back(parseLog(readLineOrThrow(file, "log line")));
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
    std::ifstream file(resolveDataPath(filename));
    return file.good();
}
