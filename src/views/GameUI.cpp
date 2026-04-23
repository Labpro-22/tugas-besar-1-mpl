#include <iostream>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <sstream>

#include "core/TurnManager.hpp"
#include "models/cards/SkillCard.hpp"
#include "models/tiles/Tile.hpp"
#include "utils/exceptions/ExceptionHandler.hpp"
#include "views/GameUI.hpp"

namespace {
    void throwIfInputClosed() {
        if (std::cin.eof()) {
            throw std::runtime_error("Input dihentikan sebelum perintah selesai diproses.");
        }
    }

    bool hasUsableSkillCard(const Player& player) {
        for (SkillCard* card : player.getHand()) {
            if (card != nullptr && (!player.isJailed() || card->canUseWhileJailed())) {
                return true;
            }
        }
        return false;
    }

    std::string trimWhitespace(const std::string& value) {
        std::size_t begin = 0;
        while (begin < value.size() &&
               std::isspace(static_cast<unsigned char>(value[begin]))) {
            ++begin;
        }

        std::size_t end = value.size();
        while (end > begin &&
               std::isspace(static_cast<unsigned char>(value[end - 1]))) {
            --end;
        }

        return value.substr(begin, end - begin);
    }

    std::string toLowercase(const std::string& value) {
        std::string result = value;
        std::transform(
            result.begin(),
            result.end(),
            result.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
        );
        return result;
    }

    bool containsWhitespace(const std::string& value) {
        return std::any_of(
            value.begin(),
            value.end(),
            [](unsigned char c) { return std::isspace(c); }
        );
    }

    bool isDuplicateName(const std::vector<std::string>& names, const std::string& name) {
        const std::string normalizedName = toLowercase(name);
        return std::any_of(
            names.begin(),
            names.end(),
            [&normalizedName](const std::string& existingName) {
                return toLowercase(existingName) == normalizedName;
            }
        );
    }

    bool parseSingleInt(const std::string& text, int& value) {
        std::istringstream stream(text);
        stream >> value;
        if (!stream) {
            return false;
        }

        std::string extra;
        return !(stream >> extra);
    }

    bool readLineOrThrow(std::string& line) {
        if (std::getline(std::cin, line)) {
            return true;
        }
        throwIfInputClosed();
        return false;
    }
}

int GameUI::showMainMenu() {
    std::cout << "+------------------------------+" << std::endl;
    std::cout << "|          NIMONSPOLI          |" << std::endl;
    std::cout << "+------------------------------+" << std::endl;
    std::cout << "| 1. Game Baru                 |" << std::endl;
    std::cout << "| 2. Muat Game                 |" << std::endl;
    std::cout << "+------------------------------+" << std::endl;
    std::cout << "Catatan load: MUAT <filename> dari folder data/" << std::endl;

    int choice = 0;
    while (true) {
        std::cout << "Pilih menu (1-2): ";
        std::string input;
        readLineOrThrow(input);
        input = trimWhitespace(input);

        if (parseSingleInt(input, choice) && (choice == 1 || choice == 2)) {
            return choice;
        }

        std::cout << "Input tidak valid. Masukkan 1 untuk Game Baru atau 2 untuk Muat Game." << std::endl;
    }
}

Command GameUI::promptLoadCommand() {
    std::cout << "\nMasukkan command load sesuai spesifikasi." << std::endl;
    std::cout << "Contoh: MUAT game_sesi1.txt" << std::endl;
    std::cout << "File akan dicari dari folder data/." << std::endl;
    while (true) {
        std::cout << "> ";
        Command command = cmdParser.readCommand();
        if (!command.getKeyword().empty()) {
            return command;
        }
        std::cout << "Input tidak boleh kosong. Masukkan command yang valid." << std::endl;
    }
}

int GameUI::promptPlayerCount() {
    int n = 0;
    while (true) {
        std::cout << "\nMasukkan jumlah pemain (2-4): ";
        std::string input;
        readLineOrThrow(input);
        input = trimWhitespace(input);

        if (parseSingleInt(input, n) && n >= 2 && n <= 4) {
            return n;
        }

        std::cout << "Jumlah pemain tidak valid. Masukkan angka 2 sampai 4." << std::endl;
    }
}

std::vector<std::string> GameUI::promptPlayerNames(int n) {
    std::vector<std::string> names;

    for (int i = 0; i < n; i++) {
        std::string name;
        while (true) {
            std::cout << "Masukkan nama pemain " << (i + 1) << ": ";
            if (!std::getline(std::cin, name)) {
                throwIfInputClosed();
            }

            name = trimWhitespace(name);

            if (name.empty()) {
                std::cout << "Nama pemain tidak boleh kosong." << std::endl;
                continue;
            }

            if (name.size() < 3) {
                std::cout << "Username minimal 3 karakter." << std::endl;
                continue;
            }

            if (containsWhitespace(name)) {
                std::cout << "Username tidak boleh mengandung spasi. Gunakan underscore bila perlu." << std::endl;
                continue;
            }

            if (isDuplicateName(names, name)) {
                std::cout << "Username sudah digunakan. Masukkan username yang berbeda." << std::endl;
                continue;
            }

            break;
        }
        names.push_back(name);
    }

    return names;
}

bool GameUI::confirmYN(const std::string& message) {
    char ans;
    while (true) {
        std::cout << message << " (y/n): ";
        std::string input;
        readLineOrThrow(input);
        input = trimWhitespace(input);

        if (input.size() == 1) {
            ans = input[0];
            if (ans == 'y' || ans == 'Y') {
                return true;
            }
            if (ans == 'n' || ans == 'N') {
                return false;
            }
        }

        std::cout << "Input tidak valid. Masukkan y atau n." << std::endl;
    }
}

int GameUI::promptInt(const std::string& prompt) {
    int value = 0;

    while (true) {
        std::cout << prompt;
        std::string input;
        readLineOrThrow(input);
        input = trimWhitespace(input);

        if (parseSingleInt(input, value)) {
            return value;
        }

        std::cout << "Input tidak valid. Masukkan angka." << std::endl;
    }
}

int GameUI::promptIntInRange(const std::string& prompt, int minValue, int maxValue) {
    int value = 0;

    while (true) {
        std::cout << prompt;
        std::string input;
        readLineOrThrow(input);
        input = trimWhitespace(input);

        if (parseSingleInt(input, value) && value >= minValue && value <= maxValue) {
            return value;
        }

        std::cout << "Input tidak valid. Masukkan angka "
                  << minValue << " sampai " << maxValue << "." << std::endl;
    }
}

Command GameUI::promptPlayerCommand(const std::string& username) {
    std::cout << "\n";
    std::cout << "Bingung? ketik HELP ea...\n";
    while (true) {
        std::cout << "> [" << username << "]: ";
        Command command = cmdParser.readCommand();
        if (!command.getKeyword().empty()) {
            return command;
        }
        std::cout << "Input tidak boleh kosong. Masukkan command yang valid." << std::endl;
    }
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

void GameUI::showHelp(const Player& player) {
    std::cout << "\n";
    std::cout << "+-------------------------------------------------------------+\n";
    std::cout << "| COMMAND TERSEDIA                                            |\n";
    std::cout << "+-------------------------------------------------------------+\n";
    std::cout << "| CETAK_PAPAN             | tampilkan papan                   |\n";
    std::cout << "| CETAK_AKTA KODE         | tampilkan akta properti           |\n";
    std::cout << "| CETAK_PROPERTI          | tampilkan properti pemain         |\n";
    std::cout << "| CETAK_LOG [n]           | tampilkan log transaksi           |\n";
    std::cout << "| SIMPAN file             | simpan game ke folder data/       |\n";

    if (player.isJailed()) {
        std::cout << "| BAYAR_DENDA             | keluar dari penjara dengan denda  |\n";
        if (!player.hasUsedSkillThisTurn() && hasUsableSkillCard(player)) {
            std::cout << "| GUNAKAN_KEMAMPUAN       | pakai kartu non-pergerakan        |\n";
        }
        if (!player.hasRolledThisTurn() && player.getJailTurns() <= 3) {
            std::cout << "| LEMPAR_DADU             | coba keluar penjara dengan double |\n";
            std::cout << "| ATUR_DADU X Y           | set dadu untuk percobaan double   |\n";
        }
    } else {
        if (!player.hasRolledThisTurn()) {
            std::cout << "| LEMPAR_DADU             | lempar dadu                       |\n";
            std::cout << "| ATUR_DADU X Y           | set nilai dadu manual             |\n";
            if (!player.hasUsedSkillThisTurn() && hasUsableSkillCard(player)) {
                std::cout << "| GUNAKAN_KEMAMPUAN       | pakai kartu skill sebelum dadu    |\n";
            }
        }
        std::cout << "| GADAI / TEBUS / BANGUN  | kelola aset                       |\n";
    }

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
