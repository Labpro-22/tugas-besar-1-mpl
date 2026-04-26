#include <iostream>
#include <limits>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <sstream>

#include "core/TurnManager.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "models/tiles/Tile.hpp"
#include "utils/TextFormatter.hpp"
#include "utils/exceptions/ExceptionHandler.hpp"
#include "views/CliOutputFormatter.hpp"
#include "views/GameUI.hpp"

namespace {
    const std::size_t MAX_USERNAME_LENGTH = 12;

    void throwIfInputClosed() {
        if (std::cin.eof()) {
            throw std::runtime_error("Input dihentikan sebelum perintah selesai diproses.");
        }
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

    void printLines(const std::vector<std::string>& lines) {
        for (const std::string& line : lines) {
            std::cout << line << std::endl;
        }
    }

}

Command GameUI::readCommand() {
    std::string line;
    if (!std::getline(std::cin, line)) {
        std::cin.clear();
        return Command("KELUAR", {});
    }

    std::stringstream stream(line);
    std::string keyword;
    std::vector<std::string> args;
    stream >> keyword;

    std::string arg;
    while (stream >> arg) {
        args.push_back(arg);
    }

    return Command(keyword, args);
}

int GameUI::promptAuctionBidInput(
    const std::string& playerName,
    int balance,
    int highestBid,
    const std::string& highestBidderName
) {
    while (true) {
        std::cout << "Giliran: " << playerName << "\n";
        if (highestBid < 0) {
            std::cout << "Bid tertinggi: belum ada\n";
        } else if (highestBidderName.empty()) {
            std::cout << "Bid tertinggi: " << TextFormatter::formatMoney(highestBid) << "\n";
        } else {
            std::cout << "Bid tertinggi: " << TextFormatter::formatMoney(highestBid)
                      << " (" << highestBidderName << ")\n";
        }
        std::cout << "Aksi (PASS / BID <jumlah>)\n";
        std::cout << "> ";

        Command command = readCommand();
        const std::string keyword = command.getKeyword();

        if (keyword == "PASS" && command.getArgCount() == 0) {
            return -1;
        }

        if (keyword == "BID" && command.getArgCount() == 1) {
            int amount = 0;
            if (!parseSingleInt(command.getArg(0), amount) || amount < 0) {
                std::cout << "Format BID tidak valid. Gunakan BID <jumlah>.\n";
                continue;
            }

            if (amount > balance) {
                std::cout << "Hey hey cek dompet yaa kalo BID, BID melebihi saldo pemain.\n";
                continue;
            }

            if (highestBid >= 0 && amount <= highestBid) {
                std::cout << "Bid harus lebih besar dari penawaran tertinggi saat ini.\n";
                continue;
            }

            return amount;
        }

        std::cout << "Input tidak valid. Gunakan PASS atau BID <jumlah>.\n";
    }
}

int GameUI::showMainMenu() {
    printLines(CliOutputFormatter::formatMainMenu());

    int choice = 0;
    while (true) {
        std::cout << "\nPilih menu (1-2): ";
        std::string input;
        readLineOrThrow(input);
        input = trimWhitespace(input);

        if (parseSingleInt(input, choice) && (choice == 1 || choice == 2)) {
            return choice;
        }

        std::cout << "Inputnya salah yaa... Ketik 1 untuk mulai Game Baru atau 2 untuk melanjutkan permainan" << std::endl;
    }
}

Command GameUI::promptLoadCommand() {
    printLines(CliOutputFormatter::formatLoadCommandPrompt());
    while (true) {
        std::cout << "> ";
        Command command = readCommand();
        if (!command.getKeyword().empty()) {
            return command;
        }
        std::cout << "Ada yang bisa aku bantu? Kalo bingung ketik HELP yaa~" << std::endl;
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
            std::cout << "Masukkan Username pemain " << (i + 1) << ": ";
            if (!std::getline(std::cin, name)) {
                throwIfInputClosed();
            }

            name = trimWhitespace(name);

            if (name.empty()) {
                std::cout << "Usernamenya nggak boleh kosong yaa..." << std::endl;
                continue;
            }

            if (name.size() < 3) {
                std::cout << "Username minimal berisi 3 karakter." << std::endl;
                continue;
            }

            if (name.size() > MAX_USERNAME_LENGTH) {
                std::cout << "Username maksimal berisi " << MAX_USERNAME_LENGTH << " karakter." << std::endl;
                continue;
            }

            if (containsWhitespace(name)) {
                std::cout << "Username tidak boleh mengandung spasi. Gunakan underscore bila perlu." << std::endl;
                continue;
            }

            if (isDuplicateName(names, name)) {
                std::cout << "Username sudah digunakan oleh pemain lain. Masukkan username yang berbeda." << std::endl;
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

std::string GameUI::promptText(const std::string& prompt) {
    std::cout << prompt;
    std::string input;
    readLineOrThrow(input);
    return trimWhitespace(input);
}

int GameUI::promptAuctionBid(const PropertyTile&, const Player& bidder, int highestBid) {
    return promptAuctionBidInput(bidder.getUsername(), bidder.getBalance(), highestBid, "");
}

int GameUI::promptAuctionBid(
    const PropertyTile&,
    const Player& bidder,
    int highestBid,
    const std::string& highestBidderName
) {
    return promptAuctionBidInput(
        bidder.getUsername(),
        bidder.getBalance(),
        highestBid,
        highestBidderName
    );
}

int GameUI::promptTaxPaymentOption(
    const Player&,
    const std::string& tileName,
    int flatAmount,
    int percentage,
    int wealth,
    int percentageAmount
) {
    showMessage("Pilih opsi pembayaran " + tileName + ":");
    showMessage("1. Bayar tetap: " + TextFormatter::formatMoney(flatAmount));
    showMessage(
        "2. Bayar berdasarkan kekayaan: " + std::to_string(percentage) +
        "% dari total kekayaan " + TextFormatter::formatMoney(wealth) +
        " = " + TextFormatter::formatMoney(percentageAmount));
    return promptIntInRange("Pilih pembayaran (1-2): ", 1, 2);
}

int GameUI::promptTileSelection(const std::string& title, const std::vector<int>& validTileIndices) {
    return promptTileSelection(title, validTileIndices, false);
}

int GameUI::promptTileSelection(
    const std::string& title,
    const std::vector<int>& validTileIndices,
    bool allowCancel
) {
    if (validTileIndices.empty()) {
        return -1;
    }

    showMessage(title);

    if (allowCancel) {
        showMessage("Masukkan nomor tile sesuai daftar di atas, atau 0 untuk batal.");
        const int choice = promptIntInRange(
            "Pilih tile (0-" + std::to_string(validTileIndices.size()) + "): ",
            0,
            static_cast<int>(validTileIndices.size()));
        if (choice == 0) {
            return -1;
        }
        return validTileIndices[static_cast<std::size_t>(choice - 1)];
    }

    showMessage("Masukkan nomor tile sesuai daftar di atas.");
    const int choice = promptIntInRange(
        "Pilih tile (1-" + std::to_string(validTileIndices.size()) + "): ",
        1,
        static_cast<int>(validTileIndices.size()));
    return validTileIndices[static_cast<std::size_t>(choice - 1)];
}

Command GameUI::promptPlayerCommand(const std::string& username) {
    std::cout << "\n";
    std::cout << "Butuh bantuan? Ketik HELP yaa~\n";
    while (true) {
        std::cout << "> [" << username << "]: ";
        Command command = readCommand();
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

void GameUI::showPropertyNotice(const Player&, const PropertyTile& property) {
    (void)property;
}

void GameUI::showPaymentNotification(const std::string& title, const std::string& detail) {
    (void)title;
    (void)detail;
}

void GameUI::showAuctionNotification(const std::string& title, const std::string& detail) {
    (void)title;
    (void)detail;
}

void GameUI::showStreetPurchasePreview(
    const Player& player,
    const PropertyTile& tile,
    const StreetTile& street,
    int originalPrice,
    int finalPrice
) {
    printLines(CliOutputFormatter::formatStreetPurchasePreview(tile, street));
    showMessage("Uang kamu saat ini: " + TextFormatter::formatMoney(player.getBalance()));
    if (finalPrice != originalPrice) {
        showMessage(
            "Selamat!!! Diskon kamu aktif. Harga beli menjadi " + TextFormatter::formatMoney(finalPrice) +
            " dari " + TextFormatter::formatMoney(originalPrice) + "."
        );
    }
}

void GameUI::showHelp(const Player& player) {
    printLines(CliOutputFormatter::formatHelpCommands(player));
}

void GameUI::showSection(const std::string& title) {
    printLines(CliOutputFormatter::formatSectionTitle(title));
}

void GameUI::showTurnSummary(const Player& player) {
    printLines(CliOutputFormatter::formatTurnSummary(player));
}

void GameUI::showDiceLanding(
    int die1,
    int die2,
    int total,
    const std::string& playerName,
    const std::string& tileName,
    const std::string& tileCode
) {
    printLines(CliOutputFormatter::formatDiceLanding(die1, die2, total, playerName, tileName, tileCode));
}

void GameUI::showWinner(
    const std::vector<Player*>& winners,
    const std::vector<Player>& players,
    GameContext& context
) {
    TurnManager* turnManager = context.getTurnManager();
    bool maxTurnReached = turnManager != nullptr && turnManager->isMaxTurnReached();
    showWinner(winners, players, maxTurnReached);
}

void GameUI::showWinner(
    const std::vector<Player*>& winners,
    const std::vector<Player>& players,
    bool maxTurnReached
) {
    printLines(CliOutputFormatter::formatWinnerSummary(winners, players, maxTurnReached));
}

void GameUI::showLogEntries(const std::vector<LogEntry>& entries) {
    printLines(CliOutputFormatter::formatLogEntries(entries));
}

void GameUI::renderBoard(const Board& board, const std::vector<Player>& players, const TurnManager& turnManager) {
    boardRenderer.render(board, players, turnManager);
}

void GameUI::showPropertyDeed(const PropertyTile* property) {
    propRenderer.renderDeed(property);
}

void GameUI::showPlayerProperties(const Player& player) {
    propRenderer.renderPlayerProperties(player);
}
