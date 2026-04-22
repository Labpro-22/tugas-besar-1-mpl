#include <iostream>
#include <string>
#include <vector>

#include "core/GameEngine.hpp"
#include "models/state/Command.hpp"
#include "models/state/GameState.hpp"
#include "models/state/PlayerState.hpp"
#include "models/config/ConfigData.hpp"
#include "utils/ConfigLoader.hpp"
#include "utils/SaveManager.hpp"
#include "utils/TransactionLogger.hpp"
#include "views/GameUI.hpp"

int main() {
    try {
        TransactionLogger logger;
        ConfigData config = ConfigLoader("config").loadAll();
        GameEngine engine(&logger);
        GameUI ui;

        int choice = ui.showMainMenu();

        if (choice == 1) {
            ui.showMessage("Memulai game baru...");
            int playerCount = ui.promptPlayerCount();
            std::vector<std::string> playerNames = ui.promptPlayerNames(playerCount);
            engine.initialize(config, playerNames);
            ui.showMessage("Game baru berhasil diinisialisasi.");
            engine.runGameLoop();
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
                GameState gameState = SaveManager().loadGame(filename);
                std::vector<std::string> playerNames;
                for (const PlayerState& playerState : gameState.getPlayerStates()) {
                    playerNames.push_back(playerState.getUsername());
                }
                engine.initialize(config, playerNames);
                engine.loadGame(gameState);
                engine.runGameLoop();
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
