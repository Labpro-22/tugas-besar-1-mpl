#pragma once

#include <string>

#include "core/GameEngine.hpp"
#include "models/config/ConfigData.hpp"
#include "utils/TransactionLogger.hpp"
#include "views/GameUI.hpp"

class CliApplication {
public:
    CliApplication();

    int run();

private:
    ConfigData promptConfigUntilValid(const std::string& savedPath = "");
    void runNewGameFlow();
    void runLoadGameFlow();
    void runActiveGameLoop();

    TransactionLogger logger;
    GameUI ui;
    GameEngine engine;
};
