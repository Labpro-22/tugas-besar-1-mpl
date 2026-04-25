#include <iostream>
#include <string>
#include <vector>

#include "core/GameEngine.hpp"
#include "models/state/Command.hpp"
#include "models/state/GameState.hpp"
#include "models/config/ConfigData.hpp"
#include "utils/ConfigLoader.hpp"
#include "utils/SaveManager.hpp"
#include "utils/TransactionLogger.hpp"
#include "utils/exceptions/ExceptionHandler.hpp"
#include "utils/exceptions/NimonspoliException.hpp"
#include "views/GameUI.hpp"

namespace {
    void runCliGameLoop(GameEngine& engine, GameUI& ui, TransactionLogger& logger) {
        std::string displayedPlayer;
        int displayedTurn = 0;

        while (!engine.isGameEnded()) {
            Player* currentPlayer = engine.getCurrentPlayer();
            if (currentPlayer == nullptr) {
                break;
            }

            if (displayedTurn != engine.getCurrentTurn() ||
                displayedPlayer != currentPlayer->getUsername()) {
                displayedTurn = engine.getCurrentTurn();
                displayedPlayer = currentPlayer->getUsername();
                ui.showSection(
                    "TURN " + std::to_string(displayedTurn) + " - " + displayedPlayer);
                ui.showTurnSummary(*currentPlayer);
            }

            Command command = ui.promptPlayerCommand(currentPlayer->getUsername());
            if (command.getKeyword() == "KELUAR") {
                return;
            }

            try {
                engine.executeCommand(command);
            } catch (const std::exception& e) {
                const Player* activePlayer = engine.getCurrentPlayer();
                ui.showError(
                    e,
                    &logger,
                    engine.getCurrentTurn(),
                    activePlayer == nullptr ? "SYSTEM" : activePlayer->getUsername());
            } catch (...) {
                ui.showUnknownError(&logger, engine.getCurrentTurn(), "SYSTEM");
            }
        }

        ui.showSection("PERMAINAN SELESAI");
        ui.showWinner(
            engine.determineWinner(),
            engine.getPlayers(),
            engine.isMaxTurnReached());
    }
}

int main() {
    try {
        TransactionLogger logger;
        ConfigData config = ConfigLoader("config").loadAll();
        GameUI ui;
        GameEngine engine(ui, &logger);

        int choice = ui.showMainMenu();

        if (choice == 1) {
            ui.showMessage("\nMemulai game baru...");
            int playerCount = ui.promptPlayerCount();
            std::vector<std::string> playerNames = ui.promptPlayerNames(playerCount);
            engine.startNewGame(config, playerNames);
            ui.showMessage("\n\nGame baru berhasil diinisialisasi.");
            runCliGameLoop(engine, ui, logger);
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

                engine.loadGame(config, gameState);
                ui.showMessage(
                    "Permainan berhasil dimuat. Melanjutkan giliran " +
                    gameState.getActivePlayerUsername() + "..."
                );
                runCliGameLoop(engine, ui, logger);
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
