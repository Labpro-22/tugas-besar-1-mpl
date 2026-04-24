#include "core/GameEngine.hpp"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <utility>

#include "core/BoardFactory.hpp"
#include "core/DeckFactory.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/GameStateMapper.hpp"
#include "core/TurnService.hpp"
#include "models/config/ConfigData.hpp"
#include "models/config/MiscConfig.hpp"
#include "models/state/Command.hpp"
#include "models/state/GameState.hpp"
#include "models/state/PlayerState.hpp"
#include "models/state/PropertyState.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "models/tiles/Tile.hpp"
#include "utils/TransactionLogger.hpp"
#include "utils/exceptions/NimonspoliException.hpp"

namespace {
    std::vector<Player*> buildPlayerPointersFrom(std::vector<Player>& players) {
        std::vector<Player*> result;
        result.reserve(players.size());
        for (Player& player : players) {
            result.push_back(&player);
        }
        return result;
    }

    Player* findPlayerByUsername(std::vector<Player>& players, const std::string& username) {
        for (Player& player : players) {
            if (player.getUsername() == username) {
                return &player;
            }
        }
        return nullptr;
    }

    int parseSavedBuildingLevel(const std::string& value) {
        if (value == "H") {
            return 5;
        }

        try {
            std::size_t parsed = 0;
            const int level = std::stoi(value, &parsed);
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
}

GameEngine::GameEngine(GameIO& io, TransactionLogger* logger)
    : io(io),
      logger(logger),
      turnManager(0),
      commandProcessor(
          board,
          players,
          dice,
          turnManager,
          io,
          logger,
          context,
          [this]() {
              return snapshot();
          }
      )
{
}

GameEngine::~GameEngine()
{
    delete context;
}

void GameEngine::startNewGame(const ConfigData& configData, const std::vector<std::string>& playerNames)
{
    this->configData = &configData;
    resetSessionState();

    if (playerNames.size() < 2 || playerNames.size() > 4) {
        throw GameInitException("jumlah pemain harus berada pada rentang 2 sampai 4.");
    }

    players.reserve(playerNames.size());
    for (const std::string& name : playerNames) {
        players.emplace_back(name, configData.getMiscConfig().getInitialBalance());
    }

    initializeBoardAndDecks();
    randomizeTurnOrder();
    if (logger != nullptr) {
        logger->clear();
        logger->log(1, "SYSTEM", "GAME_START", "Permainan baru dimulai.");
    }

    gameReady = true;
    gameEnded = false;
    currentTurnPrepared = false;
    ensureTurnPrepared();
}

void GameEngine::loadGame(const ConfigData& configData, const GameState& gameState)
{
    this->configData = &configData;
    resetSessionState();

    BoardFactory::build(board, configData);
    DeckFactory::buildActionDecks(chanceDeck, communityDeck);
    DeckFactory::buildSkillDeckFromState(skillDeck, gameState.getDeckState());
    rebuildContext();

    for (const PlayerState& state : gameState.getPlayerStates()) {
        players.emplace_back(state.getUsername(), state.getBalance());
        Player& player = players.back();
        Tile* playerTile = board.getTile(state.getPositionCode());
        player.setPosition(playerTile == nullptr ? state.getPosition() : playerTile->getIndex());
        player.setStatus(state.getStatus());
        player.setJailTurns(state.getJailTurns());
        player.setConsecutiveDoubles(0);
        player.setShieldActive(false);
        player.setDiscountPercent(0);
        player.setUsedSkillThisTurn(false);
        player.setHasRolledThisTurn(false);
        player.setActionTakenThisTurn(false);

        for (const std::string& cardName : state.getCardHand()) {
            SkillCard* card = DeckFactory::createSkillCardByName(cardName);
            if (card == nullptr) {
                continue;
            }

            skillDeck.adoptCard(card);
            try {
                player.addCard(card);
            } catch (const CardHandFullException&) {
                skillDeck.discardCard(card);
            }
        }
    }

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
            street->setBuildingLevel(parseSavedBuildingLevel(propertyState.getBuildingLevel()));
            street->setFestivalState(
                propertyState.getFestivalMultiplier(),
                propertyState.getFestivalDuration()
            );
        }
    }

    turnManager.restoreTurnOrder(gameState.getTurnOrder(), buildPlayerPointers());
    turnManager.setMaxTurn(gameState.getMaxTurn());
    turnManager.setCurrentTurn(gameState.getCurrentTurn());

    const std::vector<Player*> activePlayers = turnManager.getActivePlayers();
    for (int index = 0; index < static_cast<int>(activePlayers.size()); ++index) {
        if (activePlayers[index] != nullptr &&
            activePlayers[index]->getUsername() == gameState.getActivePlayerUsername()) {
            turnManager.setCurrentIndex(index);
            break;
        }
    }

    if (logger != nullptr) {
        logger->clear();
        logger->importEntries(gameState.getLogEntries());
        logger->log(turnManager.getCurrentTurn(), "SYSTEM", "LOAD", "Permainan berhasil dimuat.");
    }

    gameReady = true;
    gameEnded = checkGameEnd();
    currentTurnPrepared = true;
}

bool GameEngine::executeCommand(const Command& command)
{
    if (!gameReady) {
        throw std::runtime_error("Permainan belum aktif.");
    }

    ensureTurnPrepared();

    Player* currentPlayer = getCurrentPlayer();
    if (currentPlayer == nullptr) {
        throw std::runtime_error("Tidak ada pemain aktif.");
    }

    const CommandResult result = commandProcessor.process(command, *currentPlayer);
    if (result.isTurnFinished()) {
        return finishTurnAndAdvance();
    }

    gameEnded = checkGameEnd();
    return true;
}

bool GameEngine::isReady() const
{
    return gameReady;
}

bool GameEngine::isGameEnded() const
{
    return gameEnded;
}

bool GameEngine::isMaxTurnReached() const
{
    return turnManager.getMaxTurn() > 0 && turnManager.isMaxTurnReached();
}

const ConfigData& GameEngine::getConfigData() const
{
    if (configData == nullptr) {
        throw GameInitException("ConfigData belum tersedia.");
    }
    return *configData;
}

const std::vector<Player>& GameEngine::getPlayers() const
{
    return players;
}

GameState GameEngine::snapshot() const
{
    return GameStateMapper::create(board, players, turnManager, skillDeck, logger);
}

std::vector<Player*> GameEngine::determineWinner() const
{
    std::vector<Player*> candidates;
    for (const Player& player : players) {
        if (!player.isBankrupt()) {
            candidates.push_back(const_cast<Player*>(&player));
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const Player* left, const Player* right) {
        if (left->getBalance() != right->getBalance()) {
            return left->getBalance() > right->getBalance();
        }
        if (left->getProperties().size() != right->getProperties().size()) {
            return left->getProperties().size() > right->getProperties().size();
        }
        return left->getHand().size() > right->getHand().size();
    });

    std::vector<Player*> winners;
    if (candidates.empty()) {
        return winners;
    }

    winners.push_back(candidates.front());
    for (std::size_t index = 1; index < candidates.size(); ++index) {
        const Player* best = winners.front();
        const Player* current = candidates[index];
        if (best->getBalance() == current->getBalance() &&
            best->getProperties().size() == current->getProperties().size() &&
            best->getHand().size() == current->getHand().size()) {
            winners.push_back(candidates[index]);
        } else {
            break;
        }
    }

    return winners;
}

Player* GameEngine::getCurrentPlayer()
{
    return turnManager.getCurrentPlayer();
}

const Player* GameEngine::getCurrentPlayer() const
{
    return turnManager.getCurrentPlayer();
}

int GameEngine::getCurrentTurn() const
{
    return turnManager.getCurrentTurn();
}

int GameEngine::getMaxTurn() const
{
    return turnManager.getMaxTurn();
}

void GameEngine::setTurnChangedHandler(std::function<void()> handler)
{
    turnChangedHandler = std::move(handler);
}

bool GameEngine::ensureTurnPrepared()
{
    if (currentTurnPrepared) {
        return true;
    }

    return beginCurrentTurn();
}

bool GameEngine::beginCurrentTurn()
{
    Player* currentPlayer = getCurrentPlayer();
    if (currentPlayer == nullptr) {
        throw std::runtime_error("Tidak ada giliran aktif.");
    }

    TurnService::processTurn(
        *currentPlayer,
        board,
        skillDeck,
        getConfigData(),
        io,
        turnManager,
        logger
    );
    currentTurnPrepared = true;
    return true;
}

bool GameEngine::finishTurnAndAdvance()
{
    if (checkGameEnd()) {
        gameEnded = true;
        return true;
    }

    turnManager.advance();
    currentTurnPrepared = false;
    if (turnChangedHandler) {
        turnChangedHandler();
    }
    gameEnded = checkGameEnd();
    if (gameEnded) {
        return true;
    }

    return beginCurrentTurn();
}

bool GameEngine::checkGameEnd() const
{
    int activePlayers = 0;
    for (const Player& player : players) {
        if (!player.isBankrupt()) {
            ++activePlayers;
        }
    }

    return activePlayers <= 1 || isMaxTurnReached();
}

void GameEngine::rebuildContext()
{
    delete context;
    context = new GameContext(
        &board,
        &turnManager,
        &auctionManager,
        &bankruptcyHandler,
        logger,
        &io,
        &dice,
        &chanceDeck,
        &communityDeck,
        &skillDeck
    );
}

void GameEngine::resetSessionState()
{
    delete context;
    context = nullptr;

    players.clear();
    board.buildBoard({});
    chanceDeck = CardDeck<ActionCard>();
    communityDeck = CardDeck<ActionCard>();
    skillDeck = CardDeck<SkillCard>();
    if (logger != nullptr) {
        logger->clear();
    }
    turnManager = TurnManager(configData == nullptr ? 0 : configData->getMiscConfig().getMaxTurn());
    gameReady = false;
    currentTurnPrepared = false;
    gameEnded = false;
}

void GameEngine::initializeBoardAndDecks()
{
    BoardFactory::build(board, getConfigData());
    DeckFactory::buildActionDecks(chanceDeck, communityDeck);
    DeckFactory::buildSkillDeck(skillDeck);
    rebuildContext();
}

void GameEngine::randomizeTurnOrder()
{
    std::vector<Player*> playerPointers = buildPlayerPointers();
    std::shuffle(playerPointers.begin(), playerPointers.end(), std::mt19937(std::random_device{}()));
    turnManager = TurnManager(getConfigData().getMiscConfig().getMaxTurn());
    turnManager.initializeTurnOrder(playerPointers);
    rebuildContext();
}

std::vector<Player*> GameEngine::buildPlayerPointers()
{
    return buildPlayerPointersFrom(players);
}

std::vector<Player*> GameEngine::buildPlayerPointers() const
{
    return buildPlayerPointersFrom(const_cast<std::vector<Player>&>(players));
}
