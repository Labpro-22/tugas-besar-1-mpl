#include <iostream>
#include <limits>

#include "core/TurnManager.hpp"
#include "views/GameUI.hpp"

int GameUI::showMainMenu() {
    std::cout << "+------------------------------+" << std::endl;
    std::cout << "|          NIMONSPOLI          |" << std::endl;
    std::cout << "+------------------------------+" << std::endl;
    std::cout << "| 1. Game Baru                 |" << std::endl;
    std::cout << "| 2. Muat Game                 |" << std::endl;
    std::cout << "+------------------------------+" << std::endl;
    std::cout << "Catatan load: MUAT <filename>" << std::endl;

    int choice = 0;
    while (true) {
        std::cout << "Pilih menu (1-2): ";
        if (std::cin >> choice && (choice == 1 || choice == 2)) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return choice;
        }

        std::cout << "Input tidak valid. Masukkan 1 untuk Game Baru atau 2 untuk Muat Game." << std::endl;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

Command GameUI::promptLoadCommand() {
    std::cout << "\nMasukkan command load sesuai spesifikasi." << std::endl;
    std::cout << "Contoh: MUAT game_sesi1.txt" << std::endl;
    std::cout << "> ";
    return cmdParser.readCommand();
}

int GameUI::promptPlayerCount() {
    int n = 0;
    while (true) {
        std::cout << "\nMasukkan jumlah pemain (2-4): ";
        if (std::cin >> n && n >= 2 && n <= 4) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return n;
        }

        std::cout << "Jumlah pemain tidak valid. Masukkan angka 2 sampai 4." << std::endl;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

std::vector<std::string> GameUI::promptPlayerNames(int n) {
    std::vector<std::string> names;

    for (int i = 0; i < n; i++) {
        std::string name;
        do {
            std::cout << "Masukkan nama pemain " << (i + 1) << ": ";
            std::getline(std::cin, name);
            if (name.empty()) {
                std::cout << "Nama pemain tidak boleh kosong." << std::endl;
            }
        } while (name.empty());
        names.push_back(name);
    }

    return names;
}

bool GameUI::confirmYN(const std::string& message) {
    char ans;
    while (true) {
        std::cout << message << " (y/n): ";
        if (std::cin >> ans) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            if (ans == 'y' || ans == 'Y') {
                return true;
            }
            if (ans == 'n' || ans == 'N') {
                return false;
            }
        }

        std::cout << "Input tidak valid. Masukkan y atau n." << std::endl;
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

void GameUI::showMessage(const std::string& message) {
    std::cout << message << std::endl;
}

void GameUI::showWinner(const std::vector<Player*>& winners, GameContext& context) {
    std::cout << "=== Pemenang ===" << std::endl;
    if (context.getTurnManager() != nullptr) {
        std::cout << "Turn akhir: " << context.getTurnManager()->getCurrentTurn() << std::endl;
    }
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
