#pragma once

#include <functional>
#include <string>
#include <vector>

#include "core/CommandResult.hpp"

class Board;
class Dice;
class GameContext;
class GameState;
class GameUI;
class Player;
class TransactionLogger;
class TurnManager;
class Command;

class CommandProcessor {
private:
    Board& board;
    std::vector<Player>& players;
    Dice& dice;
    TurnManager& turnManager;
    GameUI& ui;
    TransactionLogger* logger;
    GameContext*& context;
    std::function<GameState()> createGameState;

    bool parseIntStrict(const std::string& text, int& value) const;
    bool isRecognizedCommand(const std::string& keyword) const;
    int getJailFine() const;
    void resolveMovement(Player& player, int totalMove);
    CommandResult payJailFine(Player& player);
    CommandResult processJailDiceAttempt(const Command& command, Player& player);
    CommandResult processDiceCommand(const Command& command, Player& player);
    void processSkillCommand(Player& player);
    void showLog(const Command& command);

public:
    CommandProcessor(
        Board& board,
        std::vector<Player>& players,
        Dice& dice,
        TurnManager& turnManager,
        GameUI& ui,
        TransactionLogger* logger,
        GameContext*& context,
        std::function<GameState()> createGameState
    );

    CommandResult process(const Command& command, Player& player);
};
