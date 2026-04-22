#pragma once

#include <string>
#include <vector>

#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "models/Player.hpp"
#include "models/state/Command.hpp"
#include "models/state/LogEntry.hpp"
#include "views/BoardRenderer.hpp"
#include "views/CommandParser.hpp"
#include "views/PropertyCardRenderer.hpp"

class TransactionLogger;

class GameUI : public GameIO {
private:
    BoardRenderer boardRenderer;
    CommandParser cmdParser;
    PropertyCardRenderer propRenderer;

public:
    int showMainMenu();
    Command promptLoadCommand();
    int promptPlayerCount();
    std::vector<std::string> promptPlayerNames(int n);

    bool confirmYN(const std::string& message) override;
    int promptInt(const std::string& prompt) override;
    int promptIntInRange(const std::string& prompt, int minValue, int maxValue) override;
    Command promptPlayerCommand(const std::string& username);
    void showMessage(const std::string& message) override;
    void showError(
        const std::exception& exception,
        TransactionLogger* logger = nullptr,
        int turn = 0,
        const std::string& username = "SYSTEM"
    ) override;
    void showUnknownError(
        TransactionLogger* logger = nullptr,
        int turn = 0,
        const std::string& username = "SYSTEM"
    );
    void showHelp();
    void showSection(const std::string& title);
    void showTurnSummary(const Player& player, int turn);
    void showDiceLanding(
        int die1,
        int die2,
        int total,
        const std::string& playerName,
        const std::string& tileName,
        const std::string& tileCode
    );
    void showWinner(const std::vector<Player*>& winners, GameContext& context);

    void showLog(const std::vector<LogEntry>& entries);
    void showLog(const std::vector<LogEntry>& entries, int n);

    BoardRenderer& getBoardRenderer();
    CommandParser& getCommandParser();
    PropertyCardRenderer& getPropertyCardRenderer();
};
