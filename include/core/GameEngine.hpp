#pragma once

#include <vector>
#include <string>

#include "models/Player.hpp"
#include "Board.hpp"
#include "Dice.hpp"
#include "TurnManager.hpp"
#include "AuctionManager.hpp"
#include "BankruptcyHandler.hpp"
#include "utils/CardDeck.hpp"

class ActionCard;
class SkillCard;
class ConfigData;
class GameState;
class Command;
class TransactionLogger;
class GameContext;

class GameEngine {
private:
    std::vector<Player> players;
    Board board;
    Dice dice;
    TurnManager turnManager;
    AuctionManager auctionManager;
    BankruptcyHandler bankruptcyHandler;
    TransactionLogger* logger;

    CardDeck<ActionCard> chanceDeck;
    CardDeck<ActionCard> communityDeck;
    CardDeck<SkillCard> skillDeck;

    GameContext* context;
    const ConfigData* configData;

    void buildBoard();
    void buildDecks();
    void randomizeTurnOrder();

public:
    GameEngine(TransactionLogger* logger);
    ~GameEngine();

    void initialize(const ConfigData& configData, const std::vector<std::string>& playerNames);

    void startNewGame();

    void loadGame(const GameState& gameState);

    void runGameLoop();

    void processTurn(Player& player);
    void processCommand(const Command& cmd, Player& player);

    bool checkGameEnd() const;

    std::vector<Player*> determineWinner() const;

    void distributeSkillCards();
};
