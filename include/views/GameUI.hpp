#pragma once

#include <string>
#include <vector>

#include "core/GameContext.hpp"
#include "models/Player.hpp"
#include "models/state/Command.hpp"
#include "models/state/LogEntry.hpp"
#include "views/BoardRenderer.hpp"
#include "views/CommandParser.hpp"
#include "views/PropertyCardRenderer.hpp"

class GameUI {
private:
    BoardRenderer boardRenderer;
    CommandParser cmdParser;
    PropertyCardRenderer propRenderer;

public:
    int showMainMenu();
    Command promptLoadCommand();
    int promptPlayerCount();
    std::vector<std::string> promptPlayerNames(int n);

    bool confirmYN(const std::string& message);
    void showMessage(const std::string& message);
    void showWinner(const std::vector<Player*>& winners, GameContext& context);

    void showLog(const std::vector<LogEntry>& entries);
    void showLog(const std::vector<LogEntry>& entries, int n);

    BoardRenderer& getBoardRenderer();
    CommandParser& getCommandParser();
    PropertyCardRenderer& getPropertyCardRenderer();
};
