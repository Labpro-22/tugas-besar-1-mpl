#include "app/CliApplication.hpp"

#include <exception>
#include <filesystem>
#include <string>
#include <vector>

#include "models/Player.hpp"
#include "models/state/Command.hpp"
#include "models/state/GameState.hpp"
#include "utils/ConfigLoader.hpp"
#include "utils/SaveManager.hpp"
#include "utils/exceptions/NimonspoliException.hpp"

CliApplication::CliApplication()
    : engine(ui, &logger)
{
}

int CliApplication::run()
{
    const int choice = ui.showMainMenu();

    if (choice == 1) {
        runNewGameFlow();
    } else if (choice == 2) {
        runLoadGameFlow();
    } else {
        ui.showMessage("Pilihan tidak valid.");
    }

    return 0;
}

ConfigData CliApplication::promptConfigUntilValid(const std::string& savedPath)
{
    std::string configPath = savedPath;
    bool triedSavedPath = false;

    while (true) {
        if (configPath.empty()) {
            configPath = ui.promptText("Masukkan folder config (Jika input kosong maka akan digunakan config default): ");
            if (configPath.empty()) {
                configPath = "config";
                ui.showMessage("Menggunakan folder config default: config");
            }
        } else if (!triedSavedPath && !savedPath.empty()) {
            ui.showMessage("Menggunakan config tersimpan: " + configPath);
            triedSavedPath = true;
        }

        std::error_code filesystemError;
        if (!std::filesystem::exists(configPath, filesystemError) || filesystemError) {
            ui.showMessage("Folder config \"" + configPath + "\" tidak ditemukan.");
            configPath.clear();
            continue;
        }
        if (!std::filesystem::is_directory(configPath, filesystemError) || filesystemError) {
            ui.showMessage("Path \"" + configPath + "\" bukan folder config.");
            configPath.clear();
            continue;
        }

        try {
            return ConfigLoader(configPath).loadAll();
        } catch (const std::exception& exception) {
            ui.showMessage(std::string("Config tidak valid: ") + exception.what());
            configPath.clear();
        }
    }
}

void CliApplication::runNewGameFlow()
{
    ui.showMessage("\nMemulai game baru...");
    ConfigData config = promptConfigUntilValid();
    const int playerCount = ui.promptPlayerCount();
    std::vector<std::string> playerNames = ui.promptPlayerNames(playerCount);
    engine.startNewGame(config, playerNames);
    ui.showMessage("\n\nGame baru berhasil diinisialisasi.");
    runActiveGameLoop();
}

void CliApplication::runLoadGameFlow()
{
    while (true) {
        Command loadCommand = ui.promptLoadCommand();
        const std::string& keyword = loadCommand.getKeyword();

        if (keyword == "KELUAR") {
            return;
        }

        if (keyword != "MUAT") {
            ui.showMessage("Load game hanya dapat dimulai dengan command MUAT <filename>.");
            continue;
        }
        if (!loadCommand.isValid()) {
            ui.showMessage("Format MUAT belum lengkap. Gunakan: " + loadCommand.getUsage());
            continue;
        }

        const std::string filename = loadCommand.getArg(0);
        SaveManager saveManager;
        if (!saveManager.fileExists(filename)) {
            ui.showMessage("File \"" + filename + "\" tidak ditemukan. Coba masukkan file lain.");
            continue;
        }

        GameState gameState;
        ui.showMessage("Memuat permainan...");
        try {
            gameState = saveManager.loadGame(filename);
        } catch (const ParseException&) {
            ui.showMessage("Gagal memuat file! File rusak atau format tidak dikenali.");
            continue;
        } catch (const SaveLoadException&) {
            ui.showMessage("Gagal memuat file! File rusak atau format tidak dikenali.");
            continue;
        }

        try {
            ConfigData config = promptConfigUntilValid(gameState.getConfigPath());
            engine.loadGame(config, gameState);
            ui.showMessage(
                "Permainan berhasil dimuat. Melanjutkan giliran " +
                gameState.getActivePlayerUsername() + "..."
            );
            runActiveGameLoop();
            break;
        } catch (const std::exception& exception) {
            ui.showMessage(std::string("Gagal melanjutkan save: ") + exception.what());
        }
    }
}

void CliApplication::runActiveGameLoop()
{
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
            engine.prepareCurrentTurn();
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
