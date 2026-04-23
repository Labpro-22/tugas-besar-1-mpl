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
#include "utils/exceptions/ExceptionHandler.hpp"
#include "utils/exceptions/NimonspoliException.hpp"
#include "views/GameUI.hpp"

int main() {
    try {
        TransactionLogger logger;
        ConfigData config = ConfigLoader("config").loadAll();
        GameEngine engine(&logger);
        GameUI ui;

        int choice = ui.showMainMenu();

        if (choice == 1) {
            ui.showMessage("\nMemulai game baru...");
            int playerCount = ui.promptPlayerCount();
            std::vector<std::string> playerNames = ui.promptPlayerNames(playerCount);
            engine.initialize(config, playerNames);
            ui.showMessage("\n\nGame baru berhasil diinisialisasi.");
            engine.runGameLoop();
        } else if (choice == 2) {
            Command loadCommand = ui.promptLoadCommand();
            const std::string& keyword = loadCommand.getKeyword();

            if (keyword != "MUAT") {
                ui.showMessage("Load game hanya dapat dimulai dengan command MUAT <filename>.");
            } else if (!loadCommand.isValid()) {
                ui.showMessage("Format MUAT belum lengkap. Gunakan: " + loadCommand.getUsage());
            } else {
                const std::string filename = loadCommand.getArg(0);
                SaveManager saveManager;
                if (!saveManager.fileExists(filename)) {
                    ui.showMessage("File \"" + filename + "\" tidak ditemukan.");
                    return 0;
                }

                ui.showMessage("Memuat permainan...");
                GameState gameState;
                try {
                    gameState = saveManager.loadGame(filename);
                } catch (const ParseException&) {
                    ui.showMessage("Gagal memuat file! File rusak atau format tidak dikenali.");
                    return 0;
                } catch (const SaveLoadException&) {
                    ui.showMessage("Gagal memuat file! File rusak atau format tidak dikenali.");
                    return 0;
                }

                std::vector<std::string> playerNames;
                for (const PlayerState& playerState : gameState.getPlayerStates()) {
                    playerNames.push_back(playerState.getUsername());
                }
                engine.initialize(config, playerNames);
                engine.loadGame(gameState);
                ui.showMessage(
                    "Permainan berhasil dimuat. Melanjutkan giliran " +
                    gameState.getActivePlayerUsername() + "..."
                );
                engine.runGameLoop();
            }
        } else {
            ui.showMessage("Pilihan tidak valid.");
        }
    } catch (const std::exception& e) {
        ExceptionHandler::handle(e);
        return 1;
    } catch (...) {
        ExceptionHandler::handleUnknown();
        return 1;
    }

    return 0;
}
