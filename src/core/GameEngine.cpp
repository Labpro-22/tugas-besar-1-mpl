#include "core/GameEngine.hpp"

#include <algorithm>
#include <array>
#include <random>
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
#include "models/cards/TeleportCard.hpp"
#include "models/config/ConfigData.hpp"
#include "models/config/MiscConfig.hpp"
#include "models/config/PropertyConfig.hpp"
#include "models/config/SpecialConfig.hpp"
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
#include "utils/TransactionLogger.hpp"
#include "utils/exceptions/CardHandFullException.hpp"
#include "utils/exceptions/InvalidCommandException.hpp"

namespace {
    Player* findPlayerByUsername(std::vector<Player>& players, const std::string& username) {
        for (std::size_t i = 0; i < players.size(); ++i) {
            if (players[i].getUsername() == username) {
                return &players[i];
            }
        }

        return nullptr;
    }

    const Player* findPlayerByUsernameConst(const std::vector<Player>& players, const std::string& username ) {
        for (std::size_t i = 0; i < players.size(); ++i) {
            if (players[i].getUsername() == username) {
                return &players[i];
            }
        }   
        return nullptr; 
    }

    SkillCard* createSkillCardByName(const std::string& typeName) {
        if (typeName == "MoveCard") {
            return new MoveCard();
        }
        if (typeName == "DiscountCard") {
            return new DiscountCard();
        }
        if (typeName == "ShieldCard") {
            return new ShieldCard();
        }
        if (typeName == "TeleportCard") {
            return new TeleportCard();
        }
        if (typeName == "LassoCard") {
            return new LassoCard();
        }
        if (typeName == "DemolitionCard") {
            return new DemolitionCard();
        }
        return nullptr;
    }

    Tile* createPropertyTile(const PropertyConfig& cfg, const std::map<int, int>& railroadRents, const std::map<int, int>& utilityMultipliers) {
        const int index = cfg.getId() - 1;

        if (cfg.getPropertyType() == PropertyType::STREET) {
            int rentTable[6];
            for (int i = 0; i < 6; ++i) {
                rentTable[i] = cfg.getRentAtLevel(i);
            }

            return new StreetTile(
                index,
                cfg.getCode(),
                cfg.getName(),
                cfg.getColorGroup(),
                cfg.getBuyPrice(),
                cfg.getMortgageValue(),
                cfg.getHotelCost(),
                cfg.getHotelCost(),
                rentTable
            );
        }

        if (cfg.getPropertyType() == PropertyType::RAILROAD) {
            return new RailroadTile(
                index,
                cfg.getCode(),
                cfg.getName(),
                cfg.getMortgageValue(),
                railroadRents
            );
        }

        return new UtilityTile(
            index,
            cfg.getCode(),
            cfg.getName(),
            cfg.getMortgageValue(),
            utilityMultipliers
        );
    }

    Tile* createFixedActionTile(int zeroBasedIndex, const ConfigData& configData) {
        const TaxConfig& taxConfig = configData.getTaxConfig();
        const SpecialConfig& specialConfig = configData.getSpecialConfig();

        if (zeroBasedIndex == 0) {
            return new GoTile(0, "GO", "Petak Mulai", specialConfig.getGoSalary()); 
        }
        if (zeroBasedIndex == 2) {
            return new CardTile(2, "DNU", "Dana Umum", CardType::COMMUNITY_CHEST);
        }
        if (zeroBasedIndex == 4) {
            return new TaxTile(
                4,
                "PPH",
                "Pajak Penghasilan",
                TaxType::PPH,
                taxConfig.getPphFlat(),
                taxConfig.getPphPercentage()
            );
        }
        if (zeroBasedIndex == 7) {
            return new FestivalTile(7, "FES", "Festival");
        }
        if (zeroBasedIndex == 10) {
            return new JailTile(10, "PEN", "Penjara", specialConfig.getJailFine());
        }
        if (zeroBasedIndex == 17) {
            return new CardTile(17, "DNU", "Dana Umum", CardType::COMMUNITY_CHEST);
        }
        if (zeroBasedIndex == 20) {
            return new FreeParkingTile(20, "PRK", "Parkir Gratis");
        }
        if (zeroBasedIndex == 30) {
            return new GoToJailTile(30, "GTJ", "Go To Jail");
        }
        if (zeroBasedIndex == 33) {
            return new TaxTile(
                33,
                "PBM",
                "Pajak Barang Mewah",
                TaxType::PBM,
                taxConfig.getPbmFlat(),
                0
            );
        }
        if (zeroBasedIndex == 36) {
            return new CardTile(36, "KSM", "Kesempatan", CardType::CHANCE);
        }
        if (zeroBasedIndex == 38) {
            return new FestivalTile(38, "FES", "Festival");
        }

        return nullptr;
    }

    std::vector<Player*> buildPlayerPointers(std::vector<Player>& players) {
        std::vector<Player*> result;
        for (std::size_t i = 0; i < players.size(); ++i) {
            result.push_back(&players[i]);
        }
        return result;
    }
}

GameEngine::GameEngine(TransactionLogger* logger)
    : turnManager(0),
      logger(logger),
      context(nullptr),
      configData(nullptr) {

} 

GameEngine::~GameEngine() {
    delete context;
}

void GameEngine::initialize(const ConfigData& configData, const std::vector<std::string>& playerNames) {
    this->configData = &configData;
    
    players.clear();
    const int initialBalance = configData.getMiscConfig().getInitialBalance();
    for (std::size_t i = 0; i < playerNames.size(); ++i) {
        players.emplace_back(playerNames[i], initialBalance);
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

    if (logger != nullptr) {
        logger->clear();
    }
}

void GameEngine::startNewGame() {
    if (configData == nullptr) {
        throw std::runtime_error("GameEngine belum diinisialisasi dengan ConfigData.");
    }

    if (players.empty()) {
        throw std::runtime_error("Tidak ada pemain untuk memulai game.");
    }

    for (std::size_t i = 0; i < players.size(); ++i) {
        players[i].setBalance(configData->getMiscConfig().getInitialBalance());
        players[i].setPosition(0),
        players[i].setStatus(PlayerStatus::ACTIVE);
        players[i].setJailTurns(0);
        players[i].setConsecutiveDoubles(0);
        players[i].setShieldActive(false);
        players[i].setDiscountPercent(0);
        players[i].setUsedSkillThisTurn(false);
        players[i].setHasRolledThisTurn(false);
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
        logger->log(
            turnManager.getCurrentTurn(),
            "SYSTEM",
            "GAME_START",
            "Permainan baru dimulai."
        );
    }
}

void GameEngine::loadGame(const GameState& gameState) {
    if (configData == nullptr) {
        throw std::runtime_error("ConfigData harus sudah tersedia sebelum load game.");
    }

    players.clear();
    const std::vector<PlayerState>& playerStates = gameState.getPlayerStates();
    for (std::size_t i = 0; i < playerStates.size(); ++i) {
        const PlayerState& ps = playerStates[i];
        players.emplace_back(ps.getUsername(), ps.getBalance());
        players.back().setBalance(ps.getBalance());
        players.back().setPosition(ps.getPosition());
        players.back().setStatus(ps.getStatus());
        players.back().setJailTurns(ps.getJailTurns());
        players.back().setConsecutiveDoubles(0);
        players.back().setShieldActive(false);
        players.back().setDiscountPercent(0);
        players.back().setUsedSkillThisTurn(false);
        players.back().setHasRolledThisTurn(false);

        const std::vector<std::string>& hand = ps.getCardHand();
        for (std::size_t j = 0; j < hand.size(); ++j) {
            SkillCard* card = createSkillCardByName(hand[j]);
            if (card != nullptr) {
                players.back().addCard(card);
            }
        }
    }

    buildBoard();
    buildDecks();

    std::vector<SkillCard*> restoreDeckCards;
    const std::vector<std::string>& deckState = gameState.getDeckState();
    for (std::size_t i = 0; i < deckState.size(); ++i) {
        SkillCard* card = createSkillCardByName(deckState[i]);
        if (card != nullptr) {
            restoreDeckCards.push_back(card);
        }
    }
    if (restoreDeckCards.empty()) {
        skillDeck.initializeDeck(restoreDeckCards);
    }

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

    const std::vector<PropertyState>& propertyStates = gameState.getPropertyStates();
    for (std::size_t i = 0; i < propertyStates.size(); ++i) {
        const PropertyState& propertyState = propertyStates[i];
        Tile* tile = board.getTile(propertyState.getCode());
        PropertyTile* property = dynamic_cast<PropertyTile*>(tile);
        if (property == nullptr) {
            continue;
        }

        property->returnToBank();

        if (propertyState.getOwnerUsername() != "BANK") {
            Player* owner = findPlayerByUsername(players, propertyState.getOwnerUsername());
            if (owner != nullptr) {
                property->transferTo(*owner);

                if (propertyState.getStatus() == PropertyStatus::MORTGAGED) {
                    property->mortgage();
                }
            }
        }

        // TODO: add setter in StreetTile for building level and festival multiplier
    }

    std::vector<Player*> playerPointers = buildPlayerPointers(players);
    turnManager.restoreTurnOrder(gameState.getTurnOrder(), playerPointers);
    turnManager.setCurrentIndex(gameState.getCurrentTurn());

    const std::vector<Player*> orderedPlayers = turnManager.getActivePlayers();
    for (int i = 0; i < static_cast<int>(orderedPlayers.size()); ++i) {
        if (orderedPlayers[i] != nullptr && orderedPlayers[i]->getUsername() == gameState.getActivePlayerUsername()) {
            turnManager.setCurrentIndex(i);
            break;
        }
    }

    if (logger != nullptr) {
        logger->clear();
        logger->importEntries(gameState.getLogEntries());
        logger->log(
            turnManager.getCurrentTurn(),
            "SYSTEM",
            "LOAD",
            "Permainan dimuat dari GameState."
        );
    }
}

void GameEngine::runGameLoop() {
    while (!checkGameEnd()) {
        Player* currentPlayer = turnManager.getCurrentPlayer();
        if (currentPlayer == nullptr) {
            break;
        }

        processTurn(*currentPlayer);
    } 
}

void GameEngine::processTurn(Player& player) {
    if (player.isBankrupt()) {
        turnManager.advance();
        return;
    }

    board.tickFestivals(player);
    player.resetTurnState();
    distributeSkillCards();

    if (logger != nullptr) {
        logger->log(
            turnManager.getCurrentTurn(),
            player.getUsername(),
            "TURN",
            "Awal giliran pemain."
        );
    }

    turnManager.advance();
}

void GameEngine::processCommand(const Command& cmd, Player& player) {
    if (!cmd.isValid()) {
        throw InvalidCommandException(cmd.getKeyword(), "format perintah tidak valid");
    }

    auto resolveMovement = [&](int totalMove) {
        const int tileCount = board.getTileCount();
        if (tileCount <= 0) {
            throw std::runtime_error("Board belum terbangun.");
        }

        const int oldPosition = player.getPosition();
        const int newPosition = (oldPosition + totalMove) % tileCount;
        const bool passedGo = player.moveTo(newPosition);

        if (passedGo) {
            Tile* goBase = board.getTile(0);
            GoTile* goTile = dynamic_cast<GoTile*>(goBase);
            if (goTile != nullptr) {
                goTile->awardSalary(player);
            }
        }

        Tile* landedTile = board.getTile(newPosition);
        if (landedTile != nullptr && context != nullptr) {
            landedTile->onLanded(player, *context);
        }
    };

    if (cmd.getKeyword() == "LEMPAR_DADU") {
        if (player.hasRolledThisTurn()) {
            throw InvalidCommandException(cmd.getKeyword(), "dadu sudah dilempar pada giliran ini");
        }

        dice.roll();
        player.setHasRolledThisTurn(true);

        const int total = dice.getTotal();
        resolveMovement(total);

        if (logger != nullptr) {
            logger->log(
                turnManager.getCurrentTurn(),
                player.getUsername(),
                "DADU",
                "Lempar: " + std::to_string(dice.getDie1()) + 
                    "+" + std::to_string(dice.getDie2()) + 
                    "=" + std::to_string(total)
            );
        }
        return;
    }

    if (cmd.getKeyword() == "ATUR_DADU") {
        if (player.hasRolledThisTurn()) {
            throw InvalidCommandException(cmd.getKeyword(), "dadu sudah dilempar pada giliran ini");
        }

        const int die1 = std::stoi(cmd.getArg(0));
        const int die2 = std::stoi(cmd.getArg(1));

        dice.setManual(die1, die2);
        player.setHasRolledThisTurn(true);

        const int total = dice.getTotal();
        resolveMovement(total);

        if (logger != nullptr) {
            logger->log(
                turnManager.getCurrentTurn(),
                player.getUsername(),
                "DADU",
                "Atur dadu: " + std::to_string(die1) +
                    "+" + std::to_string(dice.getDie2()) +
                    "=" + std::to_string(total)
            );
        }
        return;
    }

    if (cmd.getKeyword() == "GUNAKAN_KEMAMPUAN") {
        if (player.hasUsedSkillThisTurn()) {
            throw InvalidCommandException(cmd.getKeyword(), "kartu kemampuan sudah digunakan pada giliran ini");
        }

        if (player.hasRolledThisTurn()) {
            throw InvalidCommandException(cmd.getKeyword(), "kartu kemampuan hanya bisa digunakan sebelum melempar dadu");
        }

        const std::vector<SkillCard*>& hand = player.getHand();
        if (hand.empty()) {
            throw InvalidCommandException(cmd.getKeyword(), "tidak ada kartu kemampuan di tangan");
        }

        SkillCard* card = hand.front();
        if (card == nullptr) {
            throw InvalidCommandException(cmd.getKeyword(), "kartu kemampuan tidak valid");
        }

        card->use(player, *context);
        player.removeCard(card);
        player.setUsedSkillThisTurn(true);
        skillDeck.discardCard(card);

        if (logger != nullptr) {
            logger->log(
                turnManager.getCurrentTurn(),
                player.getUsername(),
                "KARTU",
                "Menggunakan " + card->getTypeName()
            );
        }
        return;
    }


    // TODO
    if (cmd.getKeyword() == "CETAK_PAPAN" ||
        cmd.getKeyword() == "CETAK_AKTA" ||
        cmd.getKeyword() == "CETAK_PROPERTI" ||
        cmd.getKeyword() == "GADAI" ||
        cmd.getKeyword() == "TEBUS" ||
        cmd.getKeyword() == "BANGUN" ||
        cmd.getKeyword() == "SIMPAN" ||
        cmd.getKeyword() == "MUAT" ||
        cmd.getKeyword() == "CETAK_LOG") {
        throw InvalidCommandException(cmd.getKeyword(), "handler command ini belum diimplementasikan di GameEngine");
    }

    throw InvalidCommandException(cmd.getKeyword(), "command tidak dikenali");
}

bool GameEngine::checkGameEnd() const {
    int activePlayers = 0;
    for (std::size_t i = 0; i < players.size(); ++i) {
        if (!players[i].isBankrupt()) {
            ++activePlayers;
        }
    }

    if (activePlayers <= 1) {
        return true;
    }

    return turnManager.getMaxTurn() > 0 && turnManager.isMaxTurnReached();
}

std::vector<Player*> GameEngine::determineWinner() const {
    std::vector<Player*> candidates;
    for (std::size_t i = 0; i < players.size(); ++i) {
        if (!players[i].isBankrupt()) {
            candidates.push_back(const_cast<Player*>(&players[i]));
        }
    }

    if (candidates.empty()) {
        return {};
    }

    auto betterPlayer = [](const Player* lhs, const Player* rhs) {
        if (lhs->getTotalWealth() != rhs->getTotalWealth()) {
            return lhs->getTotalWealth() > rhs->getTotalWealth();
        }
        if (lhs->getProperties().size() != rhs->getProperties().size()) {
            return lhs->getProperties().size() > rhs->getProperties().size();
        }
        return lhs->getHand().size() > rhs->getHand().size();
    };

    std::sort(candidates.begin(), candidates.end(), betterPlayer);

    std::vector<Player*> winners;
    winners.push_back(candidates.front());

    for (std::size_t i = 1; i < candidates.size(); ++i) {
        const Player* best = winners.front();
        const Player* current = candidates[i];

        if (best->getTotalWealth() == current->getTotalWealth() &&
            best->getProperties().size() == current->getProperties().size() &&
            best->getHand().size() == current->getHand().size()) {
                winners.push_back(candidates[i]);
        } else {
            break;
        } 
    }

    return winners;
}

void GameEngine::distributeSkillCards() {
    const std::vector<Player*> activePlayers = turnManager.getActivePlayers();

    for (std::size_t i = 0; i < activePlayers.size(); ++i) {
        Player* player = activePlayers[i];
        if (player == nullptr || player->isBankrupt()) {
            continue;
        }

        SkillCard* card = skillDeck.draw();
        try {
            player->addCard(card);

            if (logger != nullptr) {
                logger->log(
                    turnManager.getCurrentTurn(),
                    player->getUsername(),
                    "KARTU_SKILL",
                    "Mendapatkan " + card->getTypeName()
                );
            }
        } catch (const CardHandFullException&) {
            skillDeck.discardCard(card);

            if (logger != nullptr) {
                logger->log(
                    turnManager.getCurrentTurn(),
                    player->getUsername(),
                    "DROP_KARTU",
                    "Tangan penuh, kartu harus dibuang atau diproses lewat mekanisme drop."
                );
            }
        } 
    }
}

void GameEngine::buildBoard() {
    if (configData == nullptr) {
        throw std::runtime_error("ConfigData belum tersedia untuk buildBoard().");
    }

    std::vector<const PropertyConfig*> propertyByid(41, nullptr);
    const std::vector<PropertyConfig>& configs = configData->getPropertyConfigs();
    for (std::size_t i = 0; i < configs.size(); ++i) {
        const int id = configs[i].getId();
        if (id >= 1 && id <= 40) {
            propertyByid[id] = &configs[i];
        }
    }

    std::vector<Tile*> boardTiles;
    boardTiles.reserve(40);

    for (int zerobasedIndex = 0; zerobasedIndex < 40; ++zerobasedIndex) {
        Tile* fixedTile = createFixedActionTile(zerobasedIndex, *configData);
        if (fixedTile != nullptr) {
            boardTiles.push_back(fixedTile);
            continue;
        }

        const int oneBasedId = zerobasedIndex + 1;
        const PropertyConfig* cfg = propertyByid[oneBasedId];
        if (cfg == nullptr) {
            throw std::runtime_error(
                "Tidak ada konfigurasi untuk petak id " + std::to_string(oneBasedId)
            );
        }

        boardTiles.push_back(
            createPropertyTile(
                *cfg,
                configData->getRailroadRents(),
                configData->getUtilityMultipliers()
            )
        );
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
    skillCards.push_back(new MoveCard());
    skillCards.push_back(new DiscountCard());
    skillCards.push_back(new ShieldCard());
    skillCards.push_back(new TeleportCard());
    skillCards.push_back(new LassoCard());
    skillCards.push_back(new DemolitionCard());

    chanceDeck.initializeDeck(chanceCards);
    communityDeck.initializeDeck(communityCards);
    skillDeck.initializeDeck(skillCards);
}

void GameEngine::randomizeTurnOrder() {
    std::vector<Player*> playerPointers = buildPlayerPointers(players);

    std::shuffle(
        playerPointers.begin(),
        playerPointers.end(),
        std::mt19937(std::random_device{}())
    );

    turnManager.initializeTurnOrder(playerPointers);
}