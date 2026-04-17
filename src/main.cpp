#include <iostream>
#include <string>
#include <vector>

#include "core/GameEngine.hpp"
#include "views/GameUI.hpp"
#include "utils/TransactionLogger.hpp"

int main() {
    try {
        TransactionLogger logger;
        GameEngine engine(&logger);
        GameUI ui;

        ui.showMessage("Nimonspoli started.");
        ui.showMessage("Bootstrap minimal aktif.");

        int choice = ui.showMainMenu();

        if (choice == 1) {
            int playerCount = ui.promptPlayerCount();
            std::vector<std::string> playerNames = ui.promptPlayerNames(playerCount);

            for (const std::string& name : playerNames) {
                ui.showMessage("Pemain: " + name);
            }

            engine.startNewGame();
            ui.showMessage("Bootstrap berhasil. Implementasi game loop menyusul.");
        } else if (choice == 2) {
            ui.showMessage("Fitur load game belum diimplementasikan.");
        } else {
            ui.showMessage("Pilihan tidak valid.");
        }
    } catch (const std::exception& e) {
        std::cerr << "Terjadi error: " << e.what() << std::endl;
        return 1; 
    }

    return 0;
}
