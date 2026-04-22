#include <iostream>
#include <limits>
#include <stdexcept>

#include "core/TurnManager.hpp"
#include "models/tiles/Tile.hpp"
#include "utils/exceptions/ExceptionHandler.hpp"
#include "views/GameUI.hpp"

namespace {
    void throwIfInputClosed() {
        if (std::cin.eof()) {
            throw std::runtime_error("Input dihentikan sebelum perintah selesai diproses.");
        }
    }
}

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
        throwIfInputClosed();
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
        throwIfInputClosed();
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
            if (!std::getline(std::cin, name)) {
                throwIfInputClosed();
            }
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
        throwIfInputClosed();
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

int GameUI::promptInt(const std::string& prompt) {
    int value = 0;

    while (true) {
        std::cout << prompt;
        if (std::cin >> value) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }

        std::cout << "Input tidak valid. Masukkan angka." << std::endl;
        throwIfInputClosed();
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

int GameUI::promptIntInRange(const std::string& prompt, int minValue, int maxValue) {
    int value = 0;

    while (true) {
        std::cout << prompt;
        if (std::cin >> value && value >= minValue && value <= maxValue) {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            return value;
        }

        std::cout << "Input tidak valid. Masukkan angka "
                  << minValue << " sampai " << maxValue << "." << std::endl;
        throwIfInputClosed();
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
}

Command GameUI::promptPlayerCommand(const std::string& username) {
    std::cout << "\n";
    std::cout << "> [" << username << "]: ";
    return cmdParser.readCommand();
}

void GameUI::showMessage(const std::string& message) {
    std::cout << message << std::endl;
}

void GameUI::showError(
    const std::exception& exception,
    TransactionLogger* logger,
    int turn,
    const std::string& username
) {
    ExceptionHandler::handle(exception, std::cout, logger, turn, username);
}

void GameUI::showUnknownError(
    TransactionLogger* logger,
    int turn,
    const std::string& username
) {
    ExceptionHandler::handleUnknown(std::cout, logger, turn, username);
}

void GameUI::showHelp() {
    std::cout << "\n";
    std::cout << "+-------------------------------------------------------------+\n";
    std::cout << "| COMMAND                                                     |\n";
    std::cout << "+-------------------------------------------------------------+\n";
    std::cout << "| CETAK_PAPAN             | tampilkan papan                   |\n";
    std::cout << "| LEMPAR_DADU             | lempar dadu                       |\n";
    std::cout << "| ATUR_DADU X Y           | set nilai dadu manual             |\n";
    std::cout << "| CETAK_AKTA KODE         | tampilkan akta properti           |\n";
    std::cout << "| CETAK_PROPERTI          | tampilkan properti pemain         |\n";
    std::cout << "| GADAI / TEBUS / BANGUN  | kelola aset                       |\n";
    std::cout << "| GUNAKAN_KEMAMPUAN       | pakai kartu skill sebelum dadu    |\n";
    std::cout << "| SIMPAN file             | simpan game                       |\n";
    std::cout << "| MUAT file               | load hanya dari menu awal         |\n";
    std::cout << "| CETAK_LOG [n]           | tampilkan log transaksi           |\n";
    std::cout << "| HELP / KELUAR           | bantuan / keluar                  |\n";
    std::cout << "+-------------------------------------------------------------+\n";
}

void GameUI::showSection(const std::string& title) {
    std::cout << "\n";
    std::cout << "============================================================\n";
    std::cout << " " << title << "\n";
    std::cout << "============================================================\n";
}

void GameUI::showTurnSummary(const Player& player, int turn) {
    showSection("TURN " + std::to_string(turn) + " - " + player.getUsername());
    std::cout << "Saldo     : M" << player.getBalance() << "\n";
    std::cout << "Posisi    : " << (player.getPosition() + 1) << "\n";
    std::cout << "Properti  : " << player.getProperties().size() << "\n";
    std::cout << "Kartu     : " << player.getHand().size() << "\n";
}

void GameUI::showDiceLanding(
    int die1,
    int die2,
    int total,
    const std::string& playerName,
    const std::string& tileName,
    const std::string& tileCode
) {
    std::cout << "\n";
    std::cout << "Hasil: " << die1 << " + " << die2 << " = " << total << "\n";
    std::cout << "Memajukan Bidak " << playerName << " sebanyak " << total << " petak...\n";
    std::cout << "Bidak mendarat di: " << tileName << " (" << tileCode << ")\n";
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
