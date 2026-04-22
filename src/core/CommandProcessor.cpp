#include "core/CommandProcessor.hpp"

#include <stdexcept>
#include <string>

#include "core/AssetManager.hpp"
#include "core/Board.hpp"
#include "core/DeckFactory.hpp"
#include "core/Dice.hpp"
#include "core/GameContext.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/cards/SkillCard.hpp"
#include "models/state/Command.hpp"
#include "models/state/GameState.hpp"
#include "models/state/LogEntry.hpp"
#include "models/tiles/GoTile.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/Tile.hpp"
#include "utils/SaveManager.hpp"
#include "utils/TransactionLogger.hpp"
#include "utils/exceptions/NimonspoliException.hpp"
#include "views/GameUI.hpp"
#include "views/PropertyCardRenderer.hpp"

CommandProcessor::CommandProcessor(
    Board& board,
    std::vector<Player>& players,
    Dice& dice,
    TurnManager& turnManager,
    GameUI& ui,
    TransactionLogger* logger,
    GameContext*& context,
    std::function<GameState()> createGameState
) : board(board),
    players(players),
    dice(dice),
    turnManager(turnManager),
    ui(ui),
    logger(logger),
    context(context),
    createGameState(createGameState) {}

bool CommandProcessor::parseIntStrict(const std::string& text, int& value) const {
    try {
        std::size_t parsed = 0;
        value = std::stoi(text, &parsed);
        return parsed == text.size();
    } catch (const std::exception&) {
        return false;
    }
}

bool CommandProcessor::isRecognizedCommand(const std::string& keyword) const {
    return keyword == "CETAK_PAPAN" ||
           keyword == "LEMPAR_DADU" ||
           keyword == "ATUR_DADU" ||
           keyword == "CETAK_AKTA" ||
           keyword == "CETAK_PROPERTI" ||
           keyword == "GADAI" ||
           keyword == "TEBUS" ||
           keyword == "BANGUN" ||
           keyword == "SIMPAN" ||
           keyword == "MUAT" ||
           keyword == "CETAK_LOG" ||
           keyword == "GUNAKAN_KEMAMPUAN" ||
           keyword == "HELP" ||
           keyword == "KELUAR";
}

void CommandProcessor::resolveMovement(Player& player, int totalMove) {
    int tileCount = board.getTileCount();
    if (tileCount <= 0) {
        throw std::runtime_error("Board belum terbangun.");
    }

    int oldPosition = player.getPosition();
    int newPosition = (oldPosition + totalMove) % tileCount;
    bool passedGo = oldPosition + totalMove >= tileCount;
    player.moveTo(newPosition);

    if (passedGo && newPosition != 0) {
        Tile* goTile = board.getTile("GO");
        if (goTile != nullptr) {
            static_cast<GoTile*>(goTile)->awardSalary(player);
        }
    }

    Tile* landedTile = board.getTile(newPosition);
    if (landedTile != nullptr && context != nullptr) {
        ui.showDiceLanding(
            dice.getDie1(),
            dice.getDie2(),
            dice.getTotal(),
            player.getUsername(),
            landedTile->getName(),
            landedTile->getCode());
        landedTile->onLanded(player, *context);
    }
}

CommandResult CommandProcessor::processDiceCommand(const Command& command, Player& player) {
    const std::string keyword = command.getKeyword();
    if (player.hasRolledThisTurn()) {
        throw InvalidCommandException(keyword, "dadu sudah dilempar pada giliran ini");
    }

    if (keyword == "ATUR_DADU") {
        if (command.getArgCount() != 2) {
            throw InvalidCommandException(keyword, "format ATUR_DADU X Y");
        }

        int die1 = 0;
        int die2 = 0;
        if (!parseIntStrict(command.getArg(0), die1) || !parseIntStrict(command.getArg(1), die2)) {
            throw InvalidCommandException(keyword, "nilai dadu harus berupa angka 1 sampai 6");
        }
        dice.setManual(die1, die2);
        ui.showMessage("Dadu diatur secara manual.");
    } else {
        ui.showMessage("Mengocok dadu...");
        dice.roll();
    }

    player.setHasRolledThisTurn(true);

    if (dice.getDie1() == dice.getDie2()) {
        player.setConsecutiveDoubles(player.getConsecutiveDoubles() + 1);
        if (player.getConsecutiveDoubles() >= 3) {
            Tile* jailTile = board.getTile("PEN");
            if (jailTile != nullptr) {
                player.moveTo(jailTile->getIndex());
            }
            player.setStatus(PlayerStatus::JAILED);
            player.setJailTurns(0);
            ui.showMessage("Kamu mendapat double tiga kali berturut-turut!");
            ui.showMessage("Bidak langsung dipindahkan ke Penjara dan giliran berakhir.");
            if (logger != nullptr) {
                logger->log(
                    turnManager.getCurrentTurn(),
                    player.getUsername(),
                    "PENJARA",
                    "Masuk penjara karena tiga kali double.");
            }
            ui.getBoardRenderer().render(board, players, turnManager);
            return CommandResult(false, true);
        }
    } else {
        player.setConsecutiveDoubles(0);
    }

    resolveMovement(player, dice.getTotal());

    if (logger != nullptr) {
        logger->log(
            turnManager.getCurrentTurn(),
            player.getUsername(),
            "DADU",
            "Lempar: " + std::to_string(dice.getDie1()) + "+" +
                std::to_string(dice.getDie2()) + "=" + std::to_string(dice.getTotal()));
    }

    ui.getBoardRenderer().render(board, players, turnManager);
    if (dice.getDie1() == dice.getDie2() &&
        player.isActive() &&
        player.getConsecutiveDoubles() > 0 &&
        player.getConsecutiveDoubles() < 3) {
        ui.showMessage("DOUBLE! Kamu mendapat giliran tambahan.");
        player.setHasRolledThisTurn(false);
        return CommandResult(false, false);
    }

    return CommandResult(false, true);
}

void CommandProcessor::processSkillCommand(Player& player) {
    if (player.hasUsedSkillThisTurn()) {
        throw InvalidCommandException(
            "GUNAKAN_KEMAMPUAN",
            "Kamu sudah menggunakan kartu kemampuan pada giliran ini. Penggunaan kartu dibatasi maksimal 1 kali dalam 1 giliran.");
    }

    if (player.hasRolledThisTurn()) {
        throw InvalidCommandException(
            "GUNAKAN_KEMAMPUAN",
            "Kartu kemampuan hanya bisa digunakan SEBELUM melempar dadu.");
    }

    const std::vector<SkillCard*>& hand = player.getHand();
    if (hand.empty()) {
        ui.showMessage("Tidak ada kartu kemampuan.");
        return;
    }

    ui.showMessage("Daftar Kartu Kemampuan Spesial Anda:");
    for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
        ui.showMessage(
            std::to_string(i + 1) + ". " + hand[i]->getTypeName() +
                " - " + DeckFactory::describeSkillCard(hand[i]));
    }
    ui.showMessage("0. Batal");

    int choice = ui.promptIntInRange(
        "Pilih kartu yang ingin digunakan (0-" + std::to_string(hand.size()) + "): ",
        0,
        static_cast<int>(hand.size()));
    if (choice == 0) {
        ui.showMessage("Penggunaan kartu dibatalkan.");
        return;
    }

    SkillCard* card = hand[choice - 1];
    card->use(player, *context);
    player.removeCard(card);
    context->getSkillDeck()->discardCard(card);
    if (logger != nullptr) {
        logger->log(
            turnManager.getCurrentTurn(),
            player.getUsername(),
            "KARTU",
            "Menggunakan " + card->getTypeName());
    }
}

void CommandProcessor::showLog(const Command& command) {
    if (logger == nullptr) {
        return;
    }

    std::vector<LogEntry> entries;
    if (command.getArgCount() == 1) {
        int count = 0;
        if (!parseIntStrict(command.getArg(0), count) || count < 0) {
            throw InvalidCommandException("CETAK_LOG", "jumlah log harus berupa angka non-negatif");
        }
        entries = logger->getLastN(count);
    } else {
        const std::vector<LogEntry>& all = logger->getAll();
        entries.assign(all.begin(), all.end());
    }

    ui.showLog(entries);
}

CommandResult CommandProcessor::process(const Command& command, Player& player) {
    std::string keyword = command.getKeyword();
    if (keyword.empty()) {
        return CommandResult();
    }

    if (!command.isValid()) {
        if (!isRecognizedCommand(keyword)) {
            throw InvalidCommandException(keyword, "command tidak dikenali");
        }
        throw InvalidCommandException(keyword, "format/argumen perintah tidak valid. Ketik HELP untuk bantuan.");
    }

    if (keyword == "HELP") {
        ui.showHelp();
        return CommandResult();
    }

    if (keyword == "KELUAR") {
        return CommandResult(true, false);
    }

    if (keyword == "CETAK_PAPAN") {
        ui.getBoardRenderer().render(board, players, turnManager);
        return CommandResult();
    }

    if (keyword == "LEMPAR_DADU" || keyword == "ATUR_DADU") {
        return processDiceCommand(command, player);
    }

    if (keyword == "CETAK_AKTA") {
        if (command.getArgCount() != 1) {
            throw InvalidCommandException(keyword, "format CETAK_AKTA KODE");
        }
        Tile* tile = board.getTile(command.getArg(0));
        PropertyTile* property = nullptr;
        if (tile != nullptr && tile->getCategory() == TileCategory::PROPERTY) {
            property = static_cast<PropertyTile*>(tile);
        }
        ui.getPropertyCardRenderer().renderDeed(property);
        return CommandResult();
    }

    if (keyword == "CETAK_PROPERTI") {
        ui.getPropertyCardRenderer().renderPlayerProperties(player);
        return CommandResult();
    }

    if (keyword == "GADAI") {
        AssetManager::mortgageProperty(player, *context);
        return CommandResult();
    }

    if (keyword == "TEBUS") {
        AssetManager::redeemProperty(player, *context);
        return CommandResult();
    }

    if (keyword == "BANGUN") {
        AssetManager::buildProperty(player, *context);
        return CommandResult();
    }

    if (keyword == "GUNAKAN_KEMAMPUAN") {
        processSkillCommand(player);
        return CommandResult();
    }

    if (keyword == "SIMPAN") {
        if (command.getArgCount() != 1) {
            throw InvalidCommandException(keyword, "format SIMPAN filename");
        }

        SaveManager().saveGame(command.getArg(0), createGameState());
        ui.showMessage("Permainan berhasil disimpan ke " + command.getArg(0));
        return CommandResult();
    }

    if (keyword == "MUAT") {
        throw InvalidCommandException(
            keyword,
            "MUAT hanya dapat dilakukan dari menu awal sebelum permainan dimulai.");
    }

    if (keyword == "CETAK_LOG") {
        showLog(command);
        return CommandResult();
    }

    throw InvalidCommandException(keyword, "command tidak dikenali");
}
