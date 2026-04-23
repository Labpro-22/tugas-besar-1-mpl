#include "core/GuiGameSession.hpp"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <utility>

#include <QString>

#include "core/BoardFactory.hpp"
#include "core/DeckFactory.hpp"
#include "core/GameStateMapper.hpp"
#include "core/TurnService.hpp"
#include "models/state/Command.hpp"
#include "models/state/GameState.hpp"
#include "models/state/PlayerState.hpp"
#include "models/state/PropertyState.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "models/tiles/Tile.hpp"
#include "utils/ConfigLoader.hpp"
#include "utils/SaveManager.hpp"
#include "utils/UiCommon.hpp"

namespace {

std::vector<Player*> buildPlayerPointersFrom(std::vector<Player>& players)
{
    std::vector<Player*> result;
    result.reserve(players.size());
    for (Player& player : players) {
        result.push_back(&player);
    }
    return result;
}

Player* findPlayerByUsername(std::vector<Player>& players, const std::string& username)
{
    for (Player& player : players) {
        if (player.getUsername() == username) {
            return &player;
        }
    }

    return nullptr;
}

int parseSavedBuildingLevel(const std::string& value)
{
    if (value == "H") {
        return 5;
    }

    std::size_t parsed = 0;
    const int level = std::stoi(value, &parsed);
    if (parsed != value.size() || level < 0 || level > 5) {
        throw std::runtime_error("Level bangunan pada save tidak valid.");
    }
    return level;
}

}  // namespace

GuiGameSession::GuiGameSession(QWidget* dialogParent)
    : turnManager(0),
      io(dialogParent),
      commandProcessor(
          board,
          players,
          dice,
          turnManager,
          io,
          &logger,
          context,
          [this]() {
              return snapshot();
          }
      )
{
}

GuiGameSession::~GuiGameSession()
{
    delete context;
}

bool GuiGameSession::loadConfig(QString* errorMessage)
{
    const QString configDir = MonopolyUi::findConfigDirectory();
    if (configDir.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Folder config tidak ditemukan.");
        }
        return false;
    }

    try {
        ConfigLoader loader(configDir.toStdString());
        configData = loader.loadAll();
        configLoaded = true;
        return true;
    } catch (const std::exception& exception) {
        if (errorMessage != nullptr) {
            *errorMessage = QString::fromStdString(exception.what());
        }
        return false;
    }
}

void GuiGameSession::setMovementStepHandler(std::function<void(const Player&, int)> handler)
{
    io.setMovementStepHandler(std::move(handler));
}

void GuiGameSession::setPropertyPurchaseHandler(std::function<bool(const Player&, const PropertyTile&)> handler)
{
    io.setPropertyPurchaseHandler(std::move(handler));
}

void GuiGameSession::setPropertyNoticeHandler(std::function<void(const Player&, const PropertyTile&)> handler)
{
    io.setPropertyNoticeHandler(std::move(handler));
}

void GuiGameSession::setBoardTileSelectionHandler(std::function<int(const QString&, const QVector<int>&, bool)> handler)
{
    io.setBoardTileSelectionHandler(std::move(handler));
}

void GuiGameSession::setTurnChangedHandler(std::function<void()> handler)
{
    turnChangedHandler = std::move(handler);
}

bool GuiGameSession::startNewGame(const std::vector<std::string>& playerNames, QString* errorMessage)
{
    if (!configLoaded && !loadConfig(errorMessage)) {
        return false;
    }

    try {
        resetSessionState();

        if (playerNames.size() < 2 || playerNames.size() > 4) {
            throw std::runtime_error("Jumlah pemain harus berada pada rentang 2 sampai 4.");
        }

        players.reserve(playerNames.size());
        for (const std::string& name : playerNames) {
            players.emplace_back(name, configData.getMiscConfig().getInitialBalance());
        }

        initializeBoardAndDecks();
        randomizeTurnOrder();
        logger.clear();
        logger.log(1, "SYSTEM", "GAME_START", "Permainan baru dimulai.");

        gameReady = true;
        gameEnded = false;
        currentTurnPrepared = false;

        return ensureTurnPrepared(errorMessage);
    } catch (const std::exception& exception) {
        return handleException(exception, errorMessage);
    }
}

bool GuiGameSession::loadGame(const std::string& filename, QString* errorMessage)
{
    if (!configLoaded && !loadConfig(errorMessage)) {
        return false;
    }

    try {
        resetSessionState();

        SaveManager saveManager;
        const GameState gameState = saveManager.loadGame(filename);

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
                } catch (const std::exception&) {
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

        logger.clear();
        logger.importEntries(gameState.getLogEntries());
        logger.log(turnManager.getCurrentTurn(), "SYSTEM", "LOAD", "Permainan berhasil dimuat.");

        gameReady = true;
        gameEnded = checkGameEnd();
        currentTurnPrepared = true;
        return true;
    } catch (const std::exception& exception) {
        return handleException(exception, errorMessage);
    }
}

bool GuiGameSession::saveGame(const std::string& filename, QString* errorMessage)
{
    return executeCommand(Command("SIMPAN", {filename}), errorMessage);
}

bool GuiGameSession::rollDice(QString* errorMessage)
{
    return executeCommand(Command("LEMPAR_DADU", {}), errorMessage);
}

bool GuiGameSession::setDiceAndRoll(int die1, int die2, QString* errorMessage)
{
    return executeCommand(
        Command("ATUR_DADU", {std::to_string(die1), std::to_string(die2)}),
        errorMessage
    );
}

bool GuiGameSession::build(QString* errorMessage)
{
    return executeCommand(Command("BANGUN", {}), errorMessage);
}

bool GuiGameSession::mortgage(QString* errorMessage)
{
    return executeCommand(Command("GADAI", {}), errorMessage);
}

bool GuiGameSession::redeem(QString* errorMessage)
{
    return executeCommand(Command("UNMORTGAGE", {}), errorMessage);
}

bool GuiGameSession::useSkill(QString* errorMessage)
{
    return executeCommand(Command("GUNAKAN_KEMAMPUAN", {}), errorMessage);
}

bool GuiGameSession::payJailFine(QString* errorMessage)
{
    return executeCommand(Command("BAYAR_DENDA", {}), errorMessage);
}

bool GuiGameSession::isReady() const
{
    return gameReady;
}

bool GuiGameSession::isGameEnded() const
{
    return gameEnded;
}

const ConfigData& GuiGameSession::getConfigData() const
{
    return configData;
}

const std::vector<Player>& GuiGameSession::getPlayers() const
{
    return players;
}

const TransactionLogger& GuiGameSession::getLogger() const
{
    return logger;
}

GameState GuiGameSession::snapshot() const
{
    return GameStateMapper::create(board, players, turnManager, skillDeck, const_cast<TransactionLogger*>(&logger));
}

std::vector<Player*> GuiGameSession::determineWinner() const
{
    std::vector<Player*> candidates;
    for (const Player& player : players) {
        if (!player.isBankrupt()) {
            candidates.push_back(const_cast<Player*>(&player));
        }
    }

    std::sort(candidates.begin(), candidates.end(), [](const Player* left, const Player* right) {
        if (left->getTotalWealth() != right->getTotalWealth()) {
            return left->getTotalWealth() > right->getTotalWealth();
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
        if (best->getTotalWealth() == current->getTotalWealth() &&
            best->getProperties().size() == current->getProperties().size() &&
            best->getHand().size() == current->getHand().size()) {
            winners.push_back(candidates[index]);
        } else {
            break;
        }
    }

    return winners;
}

Player* GuiGameSession::getCurrentPlayer()
{
    return turnManager.getCurrentPlayer();
}

const Player* GuiGameSession::getCurrentPlayer() const
{
    return turnManager.getCurrentPlayer();
}

int GuiGameSession::getCurrentTurn() const
{
    return turnManager.getCurrentTurn();
}

int GuiGameSession::getMaxTurn() const
{
    return turnManager.getMaxTurn();
}

QStringList GuiGameSession::takePendingMessages()
{
    return io.takePendingMessages();
}

QString GuiGameSession::currentPlayerUsername() const
{
    const Player* currentPlayer = getCurrentPlayer();
    if (currentPlayer == nullptr) {
        return {};
    }

    return QString::fromStdString(currentPlayer->getUsername());
}

bool GuiGameSession::executeCommand(const Command& command, QString* errorMessage)
{
    if (!gameReady) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Permainan belum aktif.");
        }
        return false;
    }

    if (!ensureTurnPrepared(errorMessage)) {
        return false;
    }

    Player* currentPlayer = getCurrentPlayer();
    if (currentPlayer == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Tidak ada pemain aktif.");
        }
        return false;
    }

    try {
        const CommandResult result = commandProcessor.process(command, *currentPlayer);
        if (result.isTurnFinished()) {
            return finishTurnAndAdvance(errorMessage);
        }

        gameEnded = checkGameEnd();
        return true;
    } catch (const std::exception& exception) {
        return handleException(exception, errorMessage);
    }
}

bool GuiGameSession::ensureTurnPrepared(QString* errorMessage)
{
    if (currentTurnPrepared) {
        return true;
    }

    return beginCurrentTurn(errorMessage);
}

bool GuiGameSession::beginCurrentTurn(QString* errorMessage)
{
    try {
        Player* currentPlayer = getCurrentPlayer();
        if (currentPlayer == nullptr) {
            throw std::runtime_error("Tidak ada giliran aktif.");
        }

        TurnService::processTurn(
            *currentPlayer,
            board,
            skillDeck,
            configData,
            io,
            turnManager,
            &logger
        );
        currentTurnPrepared = true;
        return true;
    } catch (const std::exception& exception) {
        return handleException(exception, errorMessage);
    }
}

bool GuiGameSession::finishTurnAndAdvance(QString* errorMessage)
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

    return beginCurrentTurn(errorMessage);
}

bool GuiGameSession::checkGameEnd() const
{
    int activePlayers = 0;
    for (const Player& player : players) {
        if (!player.isBankrupt()) {
            ++activePlayers;
        }
    }

    return activePlayers <= 1 || (turnManager.getMaxTurn() > 0 && turnManager.isMaxTurnReached());
}

void GuiGameSession::rebuildContext()
{
    delete context;
    context = new GameContext(
        &board,
        &turnManager,
        &auctionManager,
        &bankruptcyHandler,
        &logger,
        &io,
        &dice,
        &chanceDeck,
        &communityDeck,
        &skillDeck
    );
}

void GuiGameSession::resetSessionState()
{
    delete context;
    context = nullptr;

    players.clear();
    board.buildBoard({});
    chanceDeck = CardDeck<ActionCard>();
    communityDeck = CardDeck<ActionCard>();
    skillDeck = CardDeck<SkillCard>();
    logger.clear();
    io.clearMessages();
    turnManager = TurnManager(configData.getMiscConfig().getMaxTurn());
    gameReady = false;
    currentTurnPrepared = false;
    gameEnded = false;
}

void GuiGameSession::initializeBoardAndDecks()
{
    BoardFactory::build(board, configData);
    DeckFactory::buildActionDecks(chanceDeck, communityDeck);
    DeckFactory::buildSkillDeck(skillDeck);
    rebuildContext();
}

void GuiGameSession::randomizeTurnOrder()
{
    std::vector<Player*> playerPointers = buildPlayerPointers();
    std::shuffle(playerPointers.begin(), playerPointers.end(), std::mt19937(std::random_device{}()));
    turnManager = TurnManager(configData.getMiscConfig().getMaxTurn());
    turnManager.initializeTurnOrder(playerPointers);
    rebuildContext();
}

std::vector<Player*> GuiGameSession::buildPlayerPointers()
{
    return buildPlayerPointersFrom(players);
}

std::vector<Player*> GuiGameSession::buildPlayerPointers() const
{
    return buildPlayerPointersFrom(const_cast<std::vector<Player>&>(players));
}

bool GuiGameSession::handleException(const std::exception& exception, QString* errorMessage)
{
    if (errorMessage != nullptr) {
        *errorMessage = QString::fromStdString(exception.what());
    }

    io.showError(
        exception,
        &logger,
        turnManager.getCurrentTurn(),
        getCurrentPlayer() == nullptr ? "SYSTEM" : getCurrentPlayer()->getUsername()
    );
    return false;
}
