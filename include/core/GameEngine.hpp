#pragma once

#include <functional>
#include <string>
#include <vector>

#include "core/AuctionManager.hpp"
#include "core/BankruptcyHandler.hpp"
#include "core/Board.hpp"
#include "core/CommandProcessor.hpp"
#include "core/Dice.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/cards/ActionCard.hpp"
#include "models/cards/SkillCard.hpp"
#include "utils/CardDeck.hpp"

class Command;
class ConfigData;
class GameContext;
class GameIO;
class GameState;
class TransactionLogger;

class GameEngine {
public:
    GameEngine(GameIO& io, TransactionLogger* logger);
    ~GameEngine();

    void startNewGame(const ConfigData& configData, const std::vector<std::string>& playerNames);
    void loadGame(const ConfigData& configData, const GameState& gameState);
    bool prepareCurrentTurn();
    bool executeCommand(const Command& command);

    bool isReady() const;
    bool isGameEnded() const;
    bool isMaxTurnReached() const;
    const ConfigData& getConfigData() const;
    const std::vector<Player>& getPlayers() const;
    GameState snapshot() const;
    std::vector<Player*> determineWinner() const;
    Player* getCurrentPlayer();
    const Player* getCurrentPlayer() const;
    int getCurrentTurn() const;
    int getMaxTurn() const;
    void setTurnChangedHandler(std::function<void()> handler);

private:
    bool ensureTurnPrepared();
    bool beginCurrentTurn();
    bool finishTurnAndAdvance();
    bool checkGameEnd() const;
    void rebuildContext();
    void resetSessionState();
    void initializeBoardAndDecks();
    void randomizeTurnOrder();
    std::vector<Player*> buildPlayerPointers();
    std::vector<Player*> buildPlayerPointers() const;

    GameIO& io;
    TransactionLogger* logger;
    const ConfigData* configData = nullptr;

    Board board;
    Dice dice;
    TurnManager turnManager;
    AuctionManager auctionManager;
    BankruptcyHandler bankruptcyHandler;

    CardDeck<ActionCard> chanceDeck;
    CardDeck<ActionCard> communityDeck;
    CardDeck<SkillCard> skillDeck;

    GameContext* context = nullptr;
    std::vector<Player> players;
    CommandProcessor commandProcessor;

    bool gameReady = false;
    bool currentTurnPrepared = false;
    bool gameEnded = false;
    std::function<void()> turnChangedHandler;
};
