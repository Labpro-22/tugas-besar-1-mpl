#include "core/GameEngine.hpp"

#include <algorithm>
#include <iostream>
#include <limits>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/GameContext.hpp"
#include "models/cards/BirthdayCard.hpp"
#include "models/cards/CampaignCard.hpp"
#include "models/cards/DemolitionCard.hpp"
#include "models/cards/DiscountCard.hpp"
#include "models/cards/DoctorFeeCard.hpp"
#include "models/cards/GoToJailCard.hpp"
#include "models/cards/LassoCard.hpp"
#include "models/cards/MoveBackCard.hpp"
#include "models/cards/MoveCard.hpp"
#include "models/cards/MoveToNearestStationCard.hpp"
#include "models/cards/ShieldCard.hpp"
#include "models/cards/SkillCard.hpp"
#include "models/cards/TeleportCard.hpp"
#include "models/config/ConfigData.hpp"
#include "models/config/MiscConfig.hpp"
#include "models/config/PropertyConfig.hpp"
#include "models/config/SpecialConfig.hpp"
#include "models/config/TaxConfig.hpp"
#include "models/state/Command.hpp"
#include "models/state/GameState.hpp"
#include "models/state/PlayerState.hpp"
#include "models/state/PropertyState.hpp"
#include "models/tiles/CardTile.hpp"
#include "models/tiles/FestivalTile.hpp"
#include "models/tiles/FreeParkingTile.hpp"
#include "models/tiles/GoTile.hpp"
#include "models/tiles/GoToJailTile.hpp"
#include "models/tiles/JailTile.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/RailroadTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "models/tiles/TaxTile.hpp"
#include "models/tiles/Tile.hpp"
#include "models/tiles/UtilityTile.hpp"
#include "utils/SaveManager.hpp"
#include "utils/TransactionLogger.hpp"
#include "utils/exceptions/CardHandFullException.hpp"
#include "utils/exceptions/InsufficientFundsException.hpp"
#include "utils/exceptions/InvalidCommandException.hpp"
#include "views/BoardRenderer.hpp"
#include "views/CommandParser.hpp"
#include "views/PropertyCardRenderer.hpp"

namespace {
    Player* findPlayerByUsername(std::vector<Player>& players, const std::string& username) {
        for (Player& player : players) {
            if (player.getUsername() == username) {
                return &player;
            }
        }
        return nullptr;
    }

    SkillCard* createSkillCardByName(const std::string& typeName) {
        if (typeName == "MoveCard") return new MoveCard();
        if (typeName == "DiscountCard") return new DiscountCard(50, 1);
        if (typeName == "ShieldCard") return new ShieldCard();
        if (typeName == "TeleportCard") return new TeleportCard();
        if (typeName == "LassoCard") return new LassoCard();
        if (typeName == "DemolitionCard") return new DemolitionCard();
        return nullptr;
    }

    Tile* createPropertyTile(const PropertyConfig& config,
                            const std::map<int, int>& railroadRents,
                            const std::map<int, int>& utilityMultipliers) {
        int index = config.getId() - 1;

        if (config.getPropertyType() == PropertyType::STREET) {
            int rentTable[6];
            for (int i = 0; i < 6; ++i) {
                rentTable[i] = config.getRentAtLevel(i);
            }

            return new StreetTile(
                index,
                config.getCode(),
                config.getName(),
                config.getColorGroup(),
                config.getBuyPrice(),
                config.getMortgageValue(),
                config.getHouseCost(),
                config.getHotelCost(),
                rentTable
            );
        }

        if (config.getPropertyType() == PropertyType::RAILROAD) {
            return new RailroadTile(index, config.getCode(), config.getName(), config.getMortgageValue(), railroadRents);
        }

        return new UtilityTile(index, config.getCode(), config.getName(), config.getMortgageValue(), utilityMultipliers);
    }

    Tile* createFixedActionTile(int index, const ConfigData& configData) {
        const TaxConfig& taxConfig = configData.getTaxConfig();
        const SpecialConfig& specialConfig = configData.getSpecialConfig();

        if (index == 0) return new GoTile(0, "GO", "Petak Mulai", specialConfig.getGoSalary());
        if (index == 2) return new CardTile(2, "DNU", "Dana Umum", CardType::COMMUNITY_CHEST);
        if (index == 4) return new TaxTile(4, "PPH", "Pajak Penghasilan", TaxType::PPH, taxConfig.getPphFlat(), taxConfig.getPphPercentage());
        if (index == 7) return new FestivalTile(7, "FES", "Festival");
        if (index == 10) return new JailTile(10, "PEN", "Penjara", specialConfig.getJailFine());
        if (index == 17) return new CardTile(17, "DNU", "Dana Umum", CardType::COMMUNITY_CHEST);
        if (index == 20) return new FreeParkingTile(20, "PRK", "Parkir Gratis");
        if (index == 22) return new CardTile(22, "KSM", "Kesempatan", CardType::CHANCE);
        if (index == 30) return new GoToJailTile(30, "GTJ", "Masuk Penjara");
        if (index == 33) return new TaxTile(33, "PBM", "Pajak Barang Mewah", TaxType::PBM, taxConfig.getPbmFlat(), 0);
        if (index == 36) return new CardTile(36, "KSM", "Kesempatan", CardType::CHANCE);
        if (index == 38) return new FestivalTile(38, "FES", "Festival");

        return nullptr;
    }

    std::vector<Player*> buildPlayerPointers(std::vector<Player>& players) {
        std::vector<Player*> result;
        for (Player& player : players) {
            result.push_back(&player);
        }
        return result;
    }

    PropertyType detectPropertyType(const PropertyTile* property) {
        if (dynamic_cast<const StreetTile*>(property) != nullptr) return PropertyType::STREET;
        if (dynamic_cast<const RailroadTile*>(property) != nullptr) return PropertyType::RAILROAD;
        return PropertyType::UTILITY;
    }

    void printHelp() {
        std::cout << "\n";
        std::cout << "+-------------------------------------------------------------+\n";
        std::cout << "| COMMAND                                                     |\n";
        std::cout << "+-------------------------------------------------------------+\n";
        std::cout << "| CETAK_PAPAN             | tampilkan papan                   |\n";
        std::cout << "| LEMPAR_DADU             | lempar dadu                       |\n";
        std::cout << "| ATUR_DADU X Y           | set nilai dadu manual             |\n";
        std::cout << "| CETAK_AKTA KODE         | tampilkan akta properti           |\n";
        std::cout << "| CETAK_PROPERTI          | tampilkan properti pemain         |\n";
        std::cout << "| GADAI / TEBUS / BANGUN  | kelola aset                       |\n";
        std::cout << "| GUNAKAN_KEMAMPUAN       | pakai kartu skill sebelum dadu    |\n";
        std::cout << "| SIMPAN file / MUAT file | save/load game                    |\n";
        std::cout << "| CETAK_LOG [n]           | tampilkan log transaksi           |\n";
        std::cout << "| HELP / KELUAR           | bantuan / keluar                  |\n";
        std::cout << "+-------------------------------------------------------------+\n";
    }

    void printSection(const std::string& title) {
        std::cout << "\n";
        std::cout << "============================================================\n";
        std::cout << " " << title << "\n";
        std::cout << "============================================================\n";
    }

    int readIntInRange(const std::string& prompt, int minValue, int maxValue) {
        int value = 0;

        while (true) {
            std::cout << prompt;
            if (std::cin >> value && value >= minValue && value <= maxValue) {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                return value;
            }

            std::cout << "Input tidak valid. Masukkan angka "
                      << minValue << " sampai " << maxValue << "." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        }
    }

    bool parseIntStrict(const std::string& text, int& value) {
        try {
            std::size_t parsed = 0;
            value = std::stoi(text, &parsed);
            return parsed == text.size();
        } catch (const std::exception&) {
            return false;
        }
    }
}  // namespace

GameEngine::GameEngine(TransactionLogger* logger)
    : turnManager(0),
      logger(logger),
      context(nullptr),
      configData(nullptr) {}

GameEngine::~GameEngine() {
    delete context;
}

void GameEngine::initialize(const ConfigData& configData, const std::vector<std::string>& playerNames) {
    this->configData = &configData;

    if (playerNames.size() < 2 || playerNames.size() > 4) {
        throw std::runtime_error("Jumlah pemain harus berada pada rentang 2 sampai 4.");
    }

    players.clear();
    for (const std::string& name : playerNames) {
        players.emplace_back(name, configData.getMiscConfig().getInitialBalance());
    }

    startNewGame();
}

void GameEngine::startNewGame() {
    if (configData == nullptr) {
        throw std::runtime_error("GameEngine belum diinisialisasi dengan ConfigData.");
    }

    for (Player& player : players) {
        player.setBalance(configData->getMiscConfig().getInitialBalance());
        player.setPosition(0);
        player.setStatus(PlayerStatus::ACTIVE);
        player.setJailTurns(0);
        player.setConsecutiveDoubles(0);
        player.setShieldActive(false);
        player.setDiscountPercent(0);
        player.setUsedSkillThisTurn(false);
        player.setHasRolledThisTurn(false);
    }

    buildBoard();
    buildDecks();

    delete context;
    context = new GameContext(
        &board,
        &turnManager,
        &auctionManager,
        &bankruptcyHandler,
        logger,
        &dice,
        &chanceDeck,
        &communityDeck,
        &skillDeck
    );

    randomizeTurnOrder();
    turnManager.setCurrentTurn(1);

    if (logger != nullptr) {
        logger->clear();
        logger->log(1, "SYSTEM", "GAME_START", "Permainan baru dimulai.");
    }
}

void GameEngine::loadGame(const GameState& gameState) {
    if (configData == nullptr) {
        throw std::runtime_error("ConfigData harus tersedia sebelum load game.");
    }

    players.clear();
    for (const PlayerState& state : gameState.getPlayerStates()) {
        players.emplace_back(state.getUsername(), state.getBalance());
        Player& player = players.back();
        player.setPosition(state.getPosition());
        player.setStatus(state.getStatus());
        player.setJailTurns(state.getJailTurns());

        for (const std::string& cardName : state.getCardHand()) {
            SkillCard* card = createSkillCardByName(cardName);
            if (card != nullptr) {
                try {
                    player.addCard(card);
                } catch (const CardHandFullException&) {
                    delete card;
                }
            }
        }
    }

    buildBoard();
    buildDecks();

    delete context;
    context = new GameContext(
        &board,
        &turnManager,
        &auctionManager,
        &bankruptcyHandler,
        logger,
        &dice,
        &chanceDeck,
        &communityDeck,
        &skillDeck
    );

    for (const PropertyState& propertyState : gameState.getPropertyStates()) {
        PropertyTile* property = dynamic_cast<PropertyTile*>(board.getTile(propertyState.getCode()));
        if (property == nullptr) {
            continue;
        }

        property->returnToBank();
        Player* owner = findPlayerByUsername(players, propertyState.getOwnerUsername());
        if (owner != nullptr) {
            property->transferTo(*owner);
            if (propertyState.getStatus() == PropertyStatus::MORTGAGED) {
                property->mortgage();
            }
        }

        StreetTile* street = dynamic_cast<StreetTile*>(property);
        if (street != nullptr) {
            int level = propertyState.getBuildingLevel() == "H" ? 5 : std::stoi(propertyState.getBuildingLevel());
            street->setBuildingLevel(level);
            street->setFestivalState(propertyState.getFestivalMultiplier(), propertyState.getFestivalDuration());
        }
    }

    turnManager.restoreTurnOrder(gameState.getTurnOrder(), buildPlayerPointers(players));
    turnManager.setCurrentTurn(gameState.getCurrentTurn());

    const std::vector<Player*> activePlayers = turnManager.getActivePlayers();
    for (int i = 0; i < static_cast<int>(activePlayers.size()); ++i) {
        if (activePlayers[i] != nullptr && activePlayers[i]->getUsername() == gameState.getActivePlayerUsername()) {
            turnManager.setCurrentIndex(i);
            break;
        }
    }

    if (logger != nullptr) {
        logger->clear();
        logger->importEntries(gameState.getLogEntries());
        logger->log(turnManager.getCurrentTurn(), "SYSTEM", "LOAD", "Permainan berhasil dimuat.");
    }
}

void GameEngine::runGameLoop() {
    CommandParser parser;
    BoardRenderer boardRenderer;

    printHelp();
    while (!checkGameEnd()) {
        Player* currentPlayer = turnManager.getCurrentPlayer();
        if (currentPlayer == nullptr) {
            break;
        }

        if (currentPlayer->isBankrupt()) {
            turnManager.advance();
            continue;
        }

        processTurn(*currentPlayer);

        bool turnFinished = false;
        while (!turnFinished && !checkGameEnd()) {
            std::cout << "\n";
            std::cout << "> [" << currentPlayer->getUsername() << "]: ";
            Command command = parser.readCommand();
            std::string keyword = command.getKeyword();

            if (keyword.empty()) {
                continue;
            }

            if (keyword == "HELP") {
                printHelp();
                continue;
            }

            if (keyword == "KELUAR") {
                return;
            }

            try {
                processCommand(command, *currentPlayer);
                if (keyword == "LEMPAR_DADU" || keyword == "ATUR_DADU") {
                    boardRenderer.render(board, players, turnManager);
                    turnFinished = true;
                }
            } catch (const std::exception& e) {
                std::cout << "Error: " << e.what() << std::endl;
            }
        }

        turnManager.advance();
    }

    std::vector<Player*> winners = determineWinner();
    printSection("PERMAINAN SELESAI");
    for (Player* winner : winners) {
        if (winner != nullptr) {
            std::cout << "Pemenang: " << winner->getUsername() << std::endl;
        }
    }
}

void GameEngine::processTurn(Player& player) {
    printSection("TURN " + std::to_string(turnManager.getCurrentTurn()) + " - " + player.getUsername());
    std::cout << "Saldo     : M" << player.getBalance() << "\n";
    std::cout << "Posisi    : " << (player.getPosition() + 1) << "\n";
    std::cout << "Properti  : " << player.getProperties().size() << "\n";
    std::cout << "Kartu     : " << player.getHand().size() << "\n";

    board.tickFestivals(player);
    player.resetTurnState();

    if (player.isJailed()) {
        player.setJailTurns(player.getJailTurns() + 1);
        std::cout << player.getUsername() << " sedang di penjara. Bayar denda M"
                  << configData->getSpecialConfig().getJailFine() << " untuk keluar." << std::endl;
        int fine = configData->getSpecialConfig().getJailFine();
        if (player.canAfford(fine)) {
            player -= fine;
            player.setStatus(PlayerStatus::ACTIVE);
            player.setJailTurns(0);
            if (logger != nullptr) {
                logger->log(turnManager.getCurrentTurn(), player.getUsername(), "PENJARA", "Membayar denda M" + std::to_string(fine));
            }
        }
    }

    try {
        SkillCard* card = skillDeck.draw();
        player.addCard(card);
        std::cout << "Kartu baru: " << card->getTypeName() << "\n";
        if (logger != nullptr) {
            logger->log(turnManager.getCurrentTurn(), player.getUsername(), "KARTU_SKILL", "Mendapatkan " + card->getTypeName());
        }
    } catch (const CardHandFullException&) {
        std::cout << "Tangan kartu penuh. Kartu baru dibuang otomatis." << std::endl;
    } catch (const std::exception&) {
    }
}

void GameEngine::processCommand(const Command& cmd, Player& player) {
    std::string keyword = cmd.getKeyword();
    if (keyword.empty()) {
        throw InvalidCommandException(keyword, "command kosong");
    }

    if (!cmd.isValid()) {
        throw InvalidCommandException(keyword, "format/argumen perintah tidak valid. Ketik HELP untuk bantuan.");
    }

    auto resolveMovement = [&](int totalMove) {
        int tileCount = board.getTileCount();
        if (tileCount <= 0) {
            throw std::runtime_error("Board belum terbangun.");
        }

        int oldPosition = player.getPosition();
        int newPosition = (oldPosition + totalMove) % tileCount;
        bool passedGo = oldPosition + totalMove >= tileCount;
        player.moveTo(newPosition);

        if (passedGo && newPosition != 0) {
            GoTile* goTile = dynamic_cast<GoTile*>(board.getTile("GO"));
            if (goTile != nullptr) {
                goTile->awardSalary(player);
            }
        }

        Tile* landedTile = board.getTile(newPosition);
        if (landedTile != nullptr && context != nullptr) {
            std::cout << "\n";
            std::cout << "Hasil dadu : " << dice.getDie1() << " + " << dice.getDie2()
                      << " = " << dice.getTotal() << "\n";
            std::cout << "Mendarat   : " << landedTile->getName()
                      << " (" << landedTile->getCode() << ")\n";
            landedTile->onLanded(player, *context);
        }
    };

    if (keyword == "CETAK_PAPAN") {
        BoardRenderer().render(board, players, turnManager);
        return;
    }

    if (keyword == "LEMPAR_DADU" || keyword == "ATUR_DADU") {
        if (player.hasRolledThisTurn()) {
            throw InvalidCommandException(keyword, "dadu sudah dilempar pada giliran ini");
        }

        if (keyword == "ATUR_DADU") {
            if (cmd.getArgCount() != 2) {
                throw InvalidCommandException(keyword, "format ATUR_DADU X Y");
            }

            int die1 = 0;
            int die2 = 0;
            if (!parseIntStrict(cmd.getArg(0), die1) || !parseIntStrict(cmd.getArg(1), die2)) {
                throw InvalidCommandException(keyword, "nilai dadu harus berupa angka 1 sampai 6");
            }
            dice.setManual(die1, die2);
        } else {
            dice.roll();
        }

        player.setHasRolledThisTurn(true);
        resolveMovement(dice.getTotal());

        if (logger != nullptr) {
            logger->log(turnManager.getCurrentTurn(), player.getUsername(), "DADU",
                        "Lempar: " + std::to_string(dice.getDie1()) + "+" +
                        std::to_string(dice.getDie2()) + "=" + std::to_string(dice.getTotal()));
        }
        return;
    }

    if (keyword == "CETAK_AKTA") {
        if (cmd.getArgCount() != 1) {
            throw InvalidCommandException(keyword, "format CETAK_AKTA KODE");
        }
        PropertyTile* property = dynamic_cast<PropertyTile*>(board.getTile(cmd.getArg(0)));
        PropertyCardRenderer().renderDeed(property);
        return;
    }

    if (keyword == "CETAK_PROPERTI") {
        PropertyCardRenderer().renderPlayerProperties(player);
        return;
    }

    if (keyword == "GADAI") {
        const std::vector<PropertyTile*>& properties = player.getProperties();
        std::vector<PropertyTile*> available;
        for (PropertyTile* property : properties) {
            if (property != nullptr && property->getStatus() == PropertyStatus::OWNED) {
                available.push_back(property);
            }
        }

        if (available.empty()) {
            std::cout << "Tidak ada properti yang dapat digadaikan." << std::endl;
            return;
        }

        for (int i = 0; i < static_cast<int>(available.size()); ++i) {
            std::cout << i + 1 << ". " << available[i]->getName() << " (" << available[i]->getCode()
                      << ") - M" << available[i]->getMortgageValue() << std::endl;
        }

        int choice = readIntInRange("Pilih properti: ", 1, static_cast<int>(available.size()));

        PropertyTile* selected = available[choice - 1];
        selected->mortgage();
        player += selected->getMortgageValue();
        if (logger != nullptr) {
            logger->log(turnManager.getCurrentTurn(), player.getUsername(), "GADAI", selected->getCode());
        }

        return;
    }

    if (keyword == "TEBUS") {
        std::vector<PropertyTile*> mortgaged;
        for (PropertyTile* property : player.getProperties()) {
            if (property != nullptr && property->getStatus() == PropertyStatus::MORTGAGED) {
                mortgaged.push_back(property);
            }
        }

        if (mortgaged.empty()) {
            std::cout << "Tidak ada properti tergadai." << std::endl;
            return;
        }

        for (int i = 0; i < static_cast<int>(mortgaged.size()); ++i) {
            std::cout << i + 1 << ". " << mortgaged[i]->getName() << " (" << mortgaged[i]->getCode()
                      << ") - M" << mortgaged[i]->getMortgageValue() << std::endl;
        }

        int choice = readIntInRange("Pilih properti: ", 1, static_cast<int>(mortgaged.size()));

        PropertyTile* selected = mortgaged[choice - 1];
        player -= selected->getMortgageValue();
        selected->redeem();

        if (logger != nullptr) {
            logger->log(turnManager.getCurrentTurn(), player.getUsername(), "TEBUS", selected->getCode());
        }

        return;
    }

    if (keyword == "BANGUN") {
        std::vector<StreetTile*> buildable;
        for (PropertyTile* property : player.getProperties()) {
            StreetTile* street = dynamic_cast<StreetTile*>(property);
            if (street != nullptr && street->getStatus() == PropertyStatus::OWNED &&
                street->canBuildNext() && board.hasMonopoly(player, street->getColorGroup())) {
                buildable.push_back(street);
            }
        }

        if (buildable.empty()) {
            std::cout << "Tidak ada properti yang bisa dibangun." << std::endl;
            return;
        }

        for (int i = 0; i < static_cast<int>(buildable.size()); ++i) {
            int cost = buildable[i]->getBuildingLevel() == 4 ? buildable[i]->getHotelCost() : buildable[i]->getHouseCost();
            std::cout << i + 1 << ". " << buildable[i]->getName() << " (" << buildable[i]->getCode()
                      << ") - Biaya M" << cost << std::endl;
        }

        int choice = readIntInRange("Pilih properti: ", 1, static_cast<int>(buildable.size()));

        StreetTile* selected = buildable[choice - 1];
        int cost = selected->getBuildingLevel() == 4 ? selected->getHotelCost() : selected->getHouseCost();
        player -= cost;
        selected->build();
        if (logger != nullptr) {
            logger->log(turnManager.getCurrentTurn(), player.getUsername(), "BANGUN", selected->getCode());
        }

        return;
    }

    if (keyword == "GUNAKAN_KEMAMPUAN") {
        if (player.hasUsedSkillThisTurn() || player.hasRolledThisTurn()) {
            throw InvalidCommandException(keyword, "kartu kemampuan hanya bisa digunakan sekali sebelum lempar dadu");
        }

        const std::vector<SkillCard*>& hand = player.getHand();
        if (hand.empty()) {
            std::cout << "Tidak ada kartu kemampuan." << std::endl;
            return;
        }

        for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
            std::cout << i + 1 << ". " << hand[i]->getTypeName() << std::endl;
        }

        int choice = readIntInRange("Pilih kartu: ", 1, static_cast<int>(hand.size()));

        SkillCard* card = hand[choice - 1];
        card->use(player, *context);
        player.removeCard(card);
        skillDeck.discardCard(card);
        if (logger != nullptr) {
            logger->log(turnManager.getCurrentTurn(), player.getUsername(), "KARTU", "Menggunakan " + card->getTypeName());
        }

        return;
    }

    if (keyword == "SIMPAN") {
        if (cmd.getArgCount() != 1) {
            throw InvalidCommandException(keyword, "format SIMPAN filename");
        }

        SaveManager().saveGame(cmd.getArg(0), createGameState());
        std::cout << "Permainan berhasil disimpan ke " << cmd.getArg(0) << std::endl;
        return;
    }

    if (keyword == "MUAT") {
        if (cmd.getArgCount() != 1) {
            throw InvalidCommandException(keyword, "format MUAT filename");
        }

        GameState state = SaveManager().loadGame(cmd.getArg(0));
        loadGame(state);
        std::cout << "Permainan berhasil dimuat dari " << cmd.getArg(0) << std::endl;
        return;
    }

    if (keyword == "CETAK_LOG") {
        if (logger == nullptr) {
            return;
        }

        std::vector<LogEntry> entries;
        if (cmd.getArgCount() == 1) {
            int count = 0;
            if (!parseIntStrict(cmd.getArg(0), count) || count < 0) {
                throw InvalidCommandException(keyword, "jumlah log harus berupa angka non-negatif");
            }
            entries = logger->getLastN(count);
        } else {
            const std::vector<LogEntry>& all = logger->getAll();
            entries.assign(all.begin(), all.end());
        }

        for (const LogEntry& entry : entries) {
            std::cout << entry.toString() << std::endl;
        }

        return;
    }

    throw InvalidCommandException(keyword, "command tidak dikenali");
}

bool GameEngine::checkGameEnd() const {
    int activePlayers = 0;
    for (const Player& player : players) {
        if (!player.isBankrupt()) {
            ++activePlayers;
        }
    }

    return activePlayers <= 1 || (turnManager.getMaxTurn() > 0 && turnManager.isMaxTurnReached());
}

std::vector<Player*> GameEngine::determineWinner() const {
    std::vector<Player*> candidates;
    for (const Player& player : players) {
        if (!player.isBankrupt()) {
            candidates.push_back(const_cast<Player*>(&player));
        }
    }

    if (candidates.empty()) {
        return {};
    }

    std::sort(candidates.begin(), candidates.end(), [](const Player* lhs, const Player* rhs) {
        if (lhs->getTotalWealth() != rhs->getTotalWealth()) {
            return lhs->getTotalWealth() > rhs->getTotalWealth();
        }
        if (lhs->getProperties().size() != rhs->getProperties().size()) {
            return lhs->getProperties().size() > rhs->getProperties().size();
        }
        return lhs->getHand().size() > rhs->getHand().size();
    });

    std::vector<Player*> winners{candidates.front()};
    for (std::size_t i = 1; i < candidates.size(); ++i) {
        Player* best = winners.front();
        Player* current = candidates[i];
        if (best->getTotalWealth() == current->getTotalWealth() &&
            best->getProperties().size() == current->getProperties().size() &&
            best->getHand().size() == current->getHand().size()) {
            winners.push_back(current);
        } else {
            break;
        }
    }
    
    return winners;
}

void GameEngine::distributeSkillCards() {
    for (Player* player : turnManager.getActivePlayers()) {
        if (player == nullptr || player->isBankrupt()) {
            continue;
        }

        try {
            SkillCard* card = skillDeck.draw();
            player->addCard(card);
        } catch (const std::exception&) {
        }
    }
}

void GameEngine::buildBoard() {
    if (configData == nullptr) {
        throw std::runtime_error("ConfigData belum tersedia.");
    }

    std::vector<const PropertyConfig*> propertyById(41, nullptr);
    for (const PropertyConfig& config : configData->getPropertyConfigs()) {
        if (config.getId() >= 1 && config.getId() <= 40) {
            propertyById[config.getId()] = &config;
        }
    }

    std::vector<Tile*> boardTiles;
    boardTiles.reserve(40);
    for (int index = 0; index < 40; ++index) {
        Tile* actionTile = createFixedActionTile(index, *configData);
        if (actionTile != nullptr) {
            boardTiles.push_back(actionTile);
            continue;
        }

        const PropertyConfig* propertyConfig = propertyById[index + 1];
        if (propertyConfig == nullptr) {
            throw std::runtime_error("Konfigurasi properti tidak ditemukan untuk petak " + std::to_string(index + 1));
        }

        boardTiles.push_back(createPropertyTile(*propertyConfig, configData->getRailroadRents(), configData->getUtilityMultipliers()));
    }

    board.buildBoard(boardTiles);
}

void GameEngine::buildDecks() {
    std::vector<ActionCard*> chanceCards;
    chanceCards.push_back(new MoveToNearestStationCard());
    chanceCards.push_back(new MoveBackCard());
    chanceCards.push_back(new GoToJailCard());

    std::vector<ActionCard*> communityCards;
    communityCards.push_back(new BirthdayCard(100));
    communityCards.push_back(new DoctorFeeCard(700));
    communityCards.push_back(new CampaignCard(200));

    std::vector<SkillCard*> skillCards;
    skillCards.push_back(new MoveCard(4, 0));
    skillCards.push_back(new DiscountCard(50, 1));
    skillCards.push_back(new ShieldCard());
    skillCards.push_back(new TeleportCard());
    skillCards.push_back(new LassoCard());
    skillCards.push_back(new DemolitionCard());

    chanceDeck.initializeDeck(chanceCards);
    communityDeck.initializeDeck(communityCards);
    skillDeck.initializeDeck(skillCards);
    chanceDeck.reshuffle();
    communityDeck.reshuffle();
    skillDeck.reshuffle();
}

void GameEngine::randomizeTurnOrder() {
    std::vector<Player*> playerPointers = buildPlayerPointers(players);
    std::shuffle(playerPointers.begin(), playerPointers.end(), std::mt19937(std::random_device{}()));
    turnManager = TurnManager(configData->getMiscConfig().getMaxTurn());
    turnManager.initializeTurnOrder(playerPointers);
}

GameState GameEngine::createGameState() const {
    std::vector<PlayerState> playerStates;
    for (const Player& player : players) {
        std::vector<std::string> cards;
        for (SkillCard* card : player.getHand()) {
            if (card != nullptr) {
                cards.push_back(card->getTypeName());
            }
        }
        playerStates.emplace_back(
            player.getUsername(),
            player.getBalance(),
            player.getPosition(),
            player.getStatus(),
            player.getJailTurns(),
            cards
        );
    }

    std::vector<std::string> turnOrder;
    for (Player* player : turnManager.getActivePlayers()) {
        if (player != nullptr) {
            turnOrder.push_back(player->getUsername());
        }
    }

    std::vector<PropertyState> propertyStates;
    for (PropertyTile* property : board.getProperties()) {
        if (property == nullptr) {
            continue;
        }

        int festivalMultiplier = 1;
        int festivalDuration = 0;
        std::string buildingLevel = "0";
        StreetTile* street = dynamic_cast<StreetTile*>(property);
        if (street != nullptr) {
            festivalMultiplier = street->getFestivalMultiplier();
            festivalDuration = street->getFestivalDuration();
            buildingLevel = street->getBuildingLevel() == 5 ? "H" : std::to_string(street->getBuildingLevel());
        }

        propertyStates.emplace_back(
            property->getCode(),
            detectPropertyType(property),
            property->getOwner() == nullptr ? "BANK" : property->getOwner()->getUsername(),
            property->getStatus(),
            festivalMultiplier,
            festivalDuration,
            buildingLevel
        );
    }

    std::vector<LogEntry> logEntries;
    if (logger != nullptr) {
        const std::vector<LogEntry>& all = logger->getAll();
        logEntries.assign(all.begin(), all.end());
    }

    Player* activePlayer = turnManager.getCurrentPlayer();
    return GameState(
        turnManager.getCurrentTurn(),
        turnManager.getMaxTurn(),
        playerStates,
        turnOrder,
        activePlayer == nullptr ? "" : activePlayer->getUsername(),
        propertyStates,
        skillDeck.getDeckState(),
        logEntries
    );
}
