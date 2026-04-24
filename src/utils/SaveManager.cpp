#include "utils/SaveManager.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "models/state/GameState.hpp"
#include "models/state/LogEntry.hpp"
#include "models/state/PlayerState.hpp"
#include "models/state/PropertyState.hpp"
#include "utils/exceptions/NimonspoliException.hpp"

std::string SaveManager::resolveDataPath(const std::string& filename) const {
    std::string normalized = filename;
    for (char& c : normalized) {
        if (c == '\\') {
            c = '/';
        }
    }

    if (normalized.empty()) {
        throw SaveLoadException("memproses nama", "", "nama file tidak boleh kosong");
    }

    const std::string dataPrefix = "data/";
    if (normalized.rfind(dataPrefix, 0) == 0) {
        if (normalized.size() < 4 || normalized.substr(normalized.size() - 4) != ".txt") {
            normalized += ".txt";
        }
        return normalized;
    }

    if (normalized.size() < 4 || normalized.substr(normalized.size() - 4) != ".txt") {
        normalized += ".txt";
    }
    return dataPrefix + normalized;
}

std::string SaveManager::getResolvedDataPath(const std::string& filename) const {
    return resolveDataPath(filename);
}

bool SaveManager::fileExists(const std::string& filename) const {
    return std::filesystem::exists(resolveDataPath(filename));
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
        throw ParseException(filePath, section, "data berakhir lebih cepat dari yang diharapkan");
    }
    return line;
}

int SaveManager::parseIntStrict(const std::string& value, const std::string& fieldName) const {
    try {
        size_t parsed = 0;
        int number = std::stoi(value, &parsed);
        if (parsed != value.size()) {
            throw std::invalid_argument("invalid suffix");
        }
        return number;
    } catch (const std::exception&) {
        throw ParseException(filePath, fieldName, "nilai integer tidak valid '" + value + "'");
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
        throw ParseException(filePath, "kartu pemain", "baris kartu kosong");
    }

    if (parts[0] == "MoveCard" || parts[0] == "DiscountCard") {
        if (parts.size() < 2) {
            throw ParseException(filePath, "kartu pemain", "nilai kartu hilang untuk " + parts[0]);
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
        throw ParseException(filePath, "log", "baris log tidak valid");
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
        throw ParseException(filePath, "player state", "header pemain tidak valid");
    }

    PlayerStatus status = PlayerStatus::ACTIVE;
    if (parts[3] == "JAILED") {
        status = PlayerStatus::JAILED;
    } else if (parts[3] == "BANKRUPT") {
        status = PlayerStatus::BANKRUPT;
    } else if (parts[3] != "ACTIVE") {
        throw ParseException(filePath, "player state", "status pemain tidak valid '" + parts[3] + "'");
    }

    int cardCount = parseIntStrict(readLineOrThrow(input, "player card count"), "player.cardCount");
    if (cardCount < 0) {
        throw ParseException(filePath, "player card count", "jumlah kartu pemain tidak boleh negatif");
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
        throw ParseException(filePath, "property line", "baris properti tidak valid");
    }

    PropertyType propertyType = PropertyType::STREET;
    if (parts[1] == "railroad" || parts[1] == "RAILROAD") {
        propertyType = PropertyType::RAILROAD;
    } else if (parts[1] == "utility" || parts[1] == "UTILITY") {
        propertyType = PropertyType::UTILITY;
    } else if (parts[1] != "street" && parts[1] != "STREET") {
        throw ParseException(filePath, "property type", "tipe properti tidak valid '" + parts[1] + "'");
    }

    PropertyStatus propertyStatus = PropertyStatus::BANK;
    if (parts[3] == "OWNED") {
        propertyStatus = PropertyStatus::OWNED;
    } else if (parts[3] == "MORTGAGED") {
        propertyStatus = PropertyStatus::MORTGAGED;
    } else if (parts[3] != "BANK") {
        throw ParseException(filePath, "property status", "status properti tidak valid '" + parts[3] + "'");
    }

    int festivalMultiplier = parseIntStrict(parts[4], "property.festivalMultiplier");
    int festivalDuration = parseIntStrict(parts[5], "property.festivalDuration");
    if (festivalMultiplier < 1 || festivalDuration < 0) {
        throw ParseException(filePath, "festival state", "state festival tidak valid");
    }

    if (propertyType == PropertyType::STREET) {
        if (parts[6] != "H") {
            int buildingLevel = parseIntStrict(parts[6], "property.buildingLevel");
            if (buildingLevel < 0 || buildingLevel > 5) {
                throw ParseException(filePath, "building level", "level bangunan harus berada pada rentang 0..5");
            }
        }
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
    std::filesystem::create_directories("data");
    std::string path = resolveDataPath(filename);
    filePath = path;
    std::ofstream file(path);
    if (!file.is_open()) {
        throw SaveLoadException("menyimpan", path, "file tidak dapat dibuka untuk ditulis");
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
    filePath = path;
    std::ifstream file(path);
    if (!file.is_open()) {
        throw SaveLoadException("memuat", path, "file tidak ditemukan atau tidak dapat dibuka");
    }

    std::vector<std::string> turnParts = splitWhitespace(readLineOrThrow(file, "turn header"));
    if (turnParts.size() != 2) {
        throw ParseException(filePath, "turn header", "header giliran harus berisi currentTurn dan maxTurn");
    }
    int currentTurn = parseIntStrict(turnParts[0], "currentTurn");
    int maxTurn = parseIntStrict(turnParts[1], "maxTurn");

    int playerCount = parseIntStrict(readLineOrThrow(file, "player count"), "playerCount");
    if (playerCount < 0) {
        throw ParseException(filePath, "player count", "jumlah pemain tidak boleh negatif");
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
        throw ParseException(filePath, "property count", "jumlah properti tidak boleh negatif");
    }

    std::vector<PropertyState> properties;
    properties.reserve(static_cast<size_t>(propertyCount));
    for (int i = 0; i < propertyCount; ++i) {
        properties.push_back(parseProperty(readLineOrThrow(file, "property line")));
    }

    int deckCount = parseIntStrict(readLineOrThrow(file, "deck count"), "deckCount");
    if (deckCount < 0) {
        throw ParseException(filePath, "deck count", "jumlah kartu deck tidak boleh negatif");
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
        throw ParseException(filePath, "log count", "jumlah log tidak boleh negatif");
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
