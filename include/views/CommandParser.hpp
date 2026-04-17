#pragma once

#include "models/state/Command.hpp"
#include "models/Player.hpp"

class CommandParser {
public:
    Command readCommand();
    bool validateFormat(const Command& cmd);
    void displayPrompt(const Player& player);
};