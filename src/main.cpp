#include <iostream>
#include <string>
#include <vector>

#include "core/GameEngine.hpp"
#include "models/state/Command.hpp"
#include "utils/TransactionLogger.hpp"
#include "views/GameUI.hpp"

int main() {
    try {
        TransactionLogger logger;
        GameEngine engine(&logger);
        GameUI ui;

        int choice = ui.showMainMenu();

        if (choice == 1) {
            ui.showMessage("Memulai game baru...");
            int playerCount = ui.promptPlayerCount();
            std::vector<std::string> playerNames = ui.promptPlayerNames(playerCount);

            for (const std::string& name : playerNames) {
                ui.showMessage("Pemain: " + name);
            }

            engine.startNewGame();
            ui.showMessage("Game baru berhasil diinisialisasi.");
            ui.showMessage("Game loop utama belum diimplementasikan.");
        } else if (choice == 2) {
            Command loadCommand = ui.promptLoadCommand();
            const std::string& keyword = loadCommand.getKeyword();

            if (keyword != "MUAT") {
                ui.showMessage("Load game hanya dapat dimulai dengan command MUAT <filename>.");
            } else if (loadCommand.getArgCount() != 1) {
                ui.showMessage("Format MUAT belum lengkap. Gunakan: MUAT <filename>");
            } else {
                const std::string filename = loadCommand.getArg(0);
                ui.showMessage("Memuat permainan...");
                ui.showMessage("Alur MUAT terdeteksi, tetapi fitur load game belum diimplementasikan.");
                ui.showMessage("File target: " + filename);
            }
        } else {
            ui.showMessage("Pilihan tidak valid.");
        }
    } catch (const std::exception& e) {
        std::cerr << "Terjadi error: " << e.what() << std::endl;
        return 1; 
    }

    return 0;
}
