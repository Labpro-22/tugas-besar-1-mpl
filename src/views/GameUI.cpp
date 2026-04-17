#include <iostream>
#include "views/GameUI.hpp"

int GameUI::showMainMenu() {
    int choice;

    std::cout << "=== Nimonspoli ===" << std::endl;
    std::cout << "1. New Game" << std::endl;
    std::cout << "2. Load Game" << std::endl;
    std::cout << "Pilih menu: ";
    std::cin >> choice;
    std::cin.ignore();

    return choice;
}

int GameUI::promptPlayerCount() {
    int n;
    std::cout << "Masukkan jumlah pemain (2-4): ";
    std::cin >> n;
    std::cin.ignore();
    return n;
}

std::vector<std::string> GameUI::promptPlayerNames(int n) {
    std::vector<std::string> names;

    for (int i = 0; i < n; i++) {
        std::string name;
        std::cout << "Masukkan nama pemain " << (i + 1) << ": ";
        std::getline(std::cin, name);
        names.push_back(name);
    }

    return names;
}

bool GameUI::confirmYN(const std::string& message) {
    char ans;
    std::cout << message << " (y/n): ";
    std::cin >> ans;
    std::cin.ignore();
    return ans == 'y' || ans == 'Y';
}

void GameUI::showMessage(const std::string& message) {
    std::cout << message << std::endl;
}

void GameUI::showWinner(const std::vector<Player*>& winners, GameContext& context) {
    (void)context;
    std::cout << "=== Pemenang ===" << std::endl;
    for (Player* player : winners) {
        if (player != nullptr) {
            std::cout << "- " << player->getUsername() << std::endl;
        }
    }
}

void GameUI::showLog(const std::vector<LogEntry>& entries) {
    for (const LogEntry& entry : entries) {
        std::cout << entry.toString() << std::endl;
    }
}

void GameUI::showLog(const std::vector<LogEntry>& entries, int n) {
    int start = static_cast<int>(entries.size()) - n;
    if (start < 0) {
        start = 0;
    }

    for (int i = start; i < static_cast<int>(entries.size()); i++) {
        std::cout << entries[i].toString() << std::endl;
    }
}

BoardRenderer& GameUI::getBoardRenderer() {
    return boardRenderer;
}

CommandParser& GameUI::getCommandParser() {
    return cmdParser;
}

PropertyCardRenderer& GameUI::getPropertyCardRenderer() {
    return propRenderer;
}
