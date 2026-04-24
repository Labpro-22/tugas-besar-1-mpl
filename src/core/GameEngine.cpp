#include "core/GameEngine.hpp"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#include "core/BoardFactory.hpp"
#include "core/DeckFactory.hpp"
#include "core/GameContext.hpp"
#include "core/GameStateMapper.hpp"
#include "core/TurnService.hpp"
#include "models/cards/SkillCard.hpp"
#include "models/config/ConfigData.hpp"
#include "models/config/MiscConfig.hpp"
#include "models/state/GameState.hpp"
#include "models/state/PlayerState.hpp"
#include "models/state/PropertyState.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "utils/TransactionLogger.hpp"
#include "utils/exceptions/NimonspoliException.hpp"

namespace {
    Player* findPlayerByUsername(std::vector<Player>& players, const std::string& username) {
        for (Player& player : players) {
            if (player.getUsername() == username) {
                return &player;
            }
        }
        return nullptr;
    }

    std::vector<Player*> buildPlayerPointers(std::vector<Player>& players) {
        std::vector<Player*> result;
        for (Player& player : players) {
            result.push_back(&player);
        }
        return result;
    }

    int parseSavedBuildingLevel(const std::string& value) {
        if (value == "H") {
            return 5;
        }

        try {
            std::size_t parsed = 0;
            int level = std::stoi(value, &parsed);
            if (parsed != value.size() || level < 0 || level > 5) {
                throw ParseException("", "building level", "level bangunan harus berada pada rentang 0..5");
            }
            return level;
        } catch (const ParseException&) {
            throw;
        } catch (const std::exception&) {
            throw ParseException("", "building level", "level bangunan tidak valid '" + value + "'");
        }
    }

}  // namespace

GameEngine::GameEngine(TransactionLogger* logger)
    : turnManager(0),
      logger(logger),
      context(nullptr),
      configData(nullptr),
      commandProcessor(
          board,
          players,
          dice,
          turnManager,
          ui,
          logger,
          context,
          [this]() {
              return GameStateMapper::create(board, players, turnManager, skillDeck, this->logger);
          }
      ) {}

GameEngine::~GameEngine() {
    delete context;
}

void GameEngine::initialize(const ConfigData& configData, const std::vector<std::string>& playerNames) {
    this->configData = &configData;

    if (playerNames.size() < 2 || playerNames.size() > 4) {
        throw GameInitException("jumlah pemain harus berada pada rentang 2 sampai 4.");
    }

    players.clear();
    for (const std::string& name : playerNames) {
        players.emplace_back(name, configData.getMiscConfig().getInitialBalance());
    }

    startNewGame();
}

void GameEngine::startNewGame() {
    if (configData == nullptr) {
        throw GameInitException("GameEngine belum diinisialisasi dengan ConfigData.");
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
        player.clearHand();
    }

    BoardFactory::build(board, *configData);
    DeckFactory::buildActionDecks(chanceDeck, communityDeck);
    DeckFactory::buildSkillDeck(skillDeck);

    delete context;
    context = new GameContext(
        &board,
        &turnManager,
        &auctionManager,
        &bankruptcyHandler,
        logger,
        &ui,
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
        throw GameInitException("ConfigData harus tersedia sebelum load game.");
    }

    BoardFactory::build(board, *configData);
    DeckFactory::buildActionDecks(chanceDeck, communityDeck);
    DeckFactory::buildSkillDeckFromState(skillDeck, gameState.getDeckState());

    players.clear();
    for (const PlayerState& state : gameState.getPlayerStates()) {
        players.emplace_back(state.getUsername(), state.getBalance());
        Player& player = players.back();
        Tile* playerTile = board.getTile(state.getPositionCode());
        player.setPosition(playerTile == nullptr ? state.getPosition() : playerTile->getIndex());
        player.setStatus(state.getStatus());
        player.setJailTurns(state.getJailTurns());

        for (const std::string& cardName : state.getCardHand()) {
            SkillCard* card = DeckFactory::createSkillCardByName(cardName);
            if (card != nullptr) {
                skillDeck.adoptCard(card);
                try {
                    player.addCard(card);
                } catch (const CardHandFullException&) {
                    skillDeck.discardCard(card);
                }
            }
        }
    }

    delete context;
    context = new GameContext(
        &board,
        &turnManager,
        &auctionManager,
        &bankruptcyHandler,
        logger,
        &ui,
        &dice,
        &chanceDeck,
        &communityDeck,
        &skillDeck
    );

    for (const PropertyState& propertyState : gameState.getPropertyStates()) {
        Tile* tile = board.getTile(propertyState.getCode());
        if (tile == nullptr || tile->asPropertyTile() == nullptr) {
            continue;
        }

        PropertyTile* property = tile->asPropertyTile();
        property->returnToBank();
        Player* owner = findPlayerByUsername(players, propertyState.getOwnerUsername());
        if (owner != nullptr) {
            property->transferTo(*owner);
            if (propertyState.getStatus() == PropertyStatus::MORTGAGED) {
                property->mortgage();
            }
        }

        StreetTile* street = property->asStreetTile();
        if (street != nullptr) {
            int level = parseSavedBuildingLevel(propertyState.getBuildingLevel());
            street->setBuildingLevel(level);
            street->setFestivalState(propertyState.getFestivalMultiplier(), propertyState.getFestivalDuration());
        }
    }

    turnManager.restoreTurnOrder(gameState.getTurnOrder(), buildPlayerPointers(players));
    turnManager.setMaxTurn(gameState.getMaxTurn());
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
            Command command = ui.promptPlayerCommand(currentPlayer->getUsername());
            try {
                CommandResult result = commandProcessor.process(command, *currentPlayer);
                if (result.isExitRequested()) {
                    return;
                }
                if (result.isTurnFinished()) {
                    turnFinished = true;
                }
            } catch (const std::exception& e) {
                ui.showError(e, logger, turnManager.getCurrentTurn(), currentPlayer->getUsername());
            } catch (...) {
                ui.showUnknownError(logger, turnManager.getCurrentTurn(), currentPlayer->getUsername());
            }
        }

        turnManager.advance();
    }

    std::vector<Player*> winners = determineWinner();
    ui.showSection("PERMAINAN SELESAI");
    if (context != nullptr) {
        ui.showWinner(winners, players, *context);
    }
}

void GameEngine::processTurn(Player& player) {
    ui.showSection("TURN " + std::to_string(turnManager.getCurrentTurn()) + " - " + player.getUsername());
    TurnService::processTurn(player, board, skillDeck, *configData, ui, turnManager, logger);
    ui.showMessage("");
    ui.showTurnSummary(player);
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
        if (lhs->getBalance() != rhs->getBalance()) {
            return lhs->getBalance() > rhs->getBalance();
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
        if (best->getBalance() == current->getBalance() &&
            best->getProperties().size() == current->getProperties().size() &&
            best->getHand().size() == current->getHand().size()) {
            winners.push_back(current);
        } else {
            break;
        }
    }
    
    return winners;
}

void GameEngine::randomizeTurnOrder() {
    std::vector<Player*> playerPointers = buildPlayerPointers(players);
    std::shuffle(playerPointers.begin(), playerPointers.end(), std::mt19937(std::random_device{}()));
    turnManager = TurnManager(configData->getMiscConfig().getMaxTurn());
    turnManager.initializeTurnOrder(playerPointers);
}
