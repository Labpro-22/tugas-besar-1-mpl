#include <iostream>
#include <sstream>
#include <vector>
#include <string>

#include "views/CommandParser.hpp"

Command CommandParser::readCommand() {
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cin.clear();
        return Command("KELUAR", {});
    }

    std::stringstream ss(line);
    std::string keyword;
    std::vector<std::string> args;

    ss >> keyword;

    std::string arg;
    while (ss >> arg) {
        args.push_back(arg);
    }

    return Command(keyword, args);
}

bool CommandParser::validateFormat(const Command& cmd) {
    return !cmd.getKeyword().empty();
}

void CommandParser::displayPrompt(const Player& player) {
    std::cout << "> [" << player.getUsername() << "]: ";
}
