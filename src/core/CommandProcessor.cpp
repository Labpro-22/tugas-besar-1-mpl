#include "core/CommandProcessor.hpp"

#include <stdexcept>
#include <string>

#include "core/AssetManager.hpp"
#include "core/BankruptcyHandler.hpp"
#include "core/Board.hpp"
#include "core/DeckFactory.hpp"
#include "core/Dice.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/cards/SkillCard.hpp"
#include "models/state/Command.hpp"
#include "models/state/GameState.hpp"
#include "models/state/LogEntry.hpp"
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
    GameIO& ui,
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

namespace {
    GameUI* asTerminalUi(GameIO& io) {
        return dynamic_cast<GameUI*>(&io);
    }
}

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
           keyword == "UNMORTGAGE" ||
           keyword == "BANGUN" ||
           keyword == "SIMPAN" ||
           keyword == "MUAT" ||
           keyword == "CETAK_LOG" ||
           keyword == "BAYAR_DENDA" ||
           keyword == "GUNAKAN_KEMAMPUAN" ||
           keyword == "HELP" ||
           keyword == "KELUAR";
}

int CommandProcessor::getJailFine() const {
    Tile* jailTile = board.getTile("PEN");
    if (jailTile == nullptr) {
        return 0;
    }
    return jailTile->getJailFine();
}

void CommandProcessor::resolveMovement(Player& player, int totalMove) {
    int tileCount = board.getTileCount();
    if (tileCount <= 0) {
        throw std::runtime_error("Board belum terbangun.");
    }

    for (int step = 0; step < totalMove; ++step) {
        const int previousPosition = player.getPosition();
        const int nextPosition = (previousPosition + 1) % tileCount;
        player.moveTo(nextPosition);
        ui.showPawnStep(player, nextPosition);

        if (previousPosition + 1 >= tileCount && nextPosition != 0 && context != nullptr) {
            Tile* goTile = board.getTile("GO");
            if (goTile != nullptr) {
                goTile->onPassed(player, *context);
            }
        }
    }

    Tile* landedTile = board.getTile(player.getPosition());
    if (landedTile != nullptr && context != nullptr) {
        if (GameUI* terminalUi = asTerminalUi(ui)) {
            terminalUi->showDiceLanding(
                dice.getDie1(),
                dice.getDie2(),
                dice.getTotal(),
                player.getUsername(),
                landedTile->getName(),
                landedTile->getCode());
        }
        landedTile->onLanded(player, *context);
    }
}

CommandResult CommandProcessor::payJailFine(Player& player) {
    if (!player.isJailed()) {
        ui.showMessage("Kamu tidak sedang berada di penjara.");
        return CommandResult();
    }

    int fine = getJailFine();
    int fineToPay = player.consumeDiscountedAmount(fine);
    if (fineToPay != fine) {
        ui.showMessage(
            "Diskon diterapkan dari M" + std::to_string(fine) +
                " menjadi M" + std::to_string(fineToPay) + ".");
    }

    bool finePaid = false;
    if (fineToPay > 0 && !player.canAfford(fineToPay)) {
        ui.showMessage("Uang tidak cukup untuk membayar denda. Aset akan dilikuidasi bila memungkinkan.");
        if (context != nullptr && context->getBankruptcyHandler() != nullptr) {
            context->getBankruptcyHandler()->handleBankruptcy(player, nullptr, fineToPay, *context);
            finePaid = !player.isBankrupt();
        } else {
            player -= fineToPay;
            finePaid = true;
        }
    } else {
        player -= fineToPay;
        finePaid = true;
    }

    if (player.isBankrupt()) {
        return CommandResult(false, true);
    }

    if (!finePaid) {
        ui.showMessage("Denda belum bisa dibayar. Kamu masih berada di penjara.");
        return CommandResult();
    }

    player.setStatus(PlayerStatus::ACTIVE);
    player.setJailTurns(0);
    player.setConsecutiveDoubles(0);
    ui.showMessage("Denda M" + std::to_string(fineToPay) + " dibayar. Kamu bebas dari penjara.");

    if (logger != nullptr) {
        logger->log(
            turnManager.getCurrentTurn(),
            player.getUsername(),
            "PENJARA",
            "Membayar denda M" + std::to_string(fineToPay) + " untuk keluar dari penjara.");
    }

    return CommandResult();
}

CommandResult CommandProcessor::processJailDiceAttempt(const Command& command, Player& player) {
    const std::string keyword = command.getKeyword();
    if (!player.isJailed()) {
        return processDiceCommand(command, player);
    }

    if (player.hasRolledThisTurn()) {
        throw InvalidCommandException(keyword, "percobaan keluar penjara sudah dilakukan pada giliran ini");
    }

    if (player.getJailTurns() > 3) {
        throw InvalidCommandException(keyword, "giliran ke-4 di penjara wajib keluar dengan BAYAR_DENDA");
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
        ui.showMessage("Dadu diatur secara manual untuk percobaan keluar penjara.");
    } else {
        ui.showMessage("Mencoba keluar penjara dengan lempar dadu...");
        dice.roll();
    }

    player.setActionTakenThisTurn(true);
    player.setHasRolledThisTurn(true);
    ui.showMessage(
        "Hasil: " + std::to_string(dice.getDie1()) + " + " +
            std::to_string(dice.getDie2()) + " = " + std::to_string(dice.getTotal()));
    ui.showDiceRoll(dice.getDie1(), dice.getDie2());

    if (logger != nullptr) {
        logger->log(
            turnManager.getCurrentTurn(),
            player.getUsername(),
            "PENJARA",
            "Percobaan keluar penjara: " + std::to_string(dice.getDie1()) + "+" +
                std::to_string(dice.getDie2()) + "=" + std::to_string(dice.getTotal()));
    }

    if (!dice.isDouble()) {
        ui.showMessage("Belum double. Kamu tetap di penjara dan giliran berakhir.");
        if (player.getJailTurns() >= 3) {
            ui.showMessage("Percobaan sudah 3 kali. Giliran berikutnya wajib BAYAR_DENDA.");
        }
        if (GameUI* terminalUi = asTerminalUi(ui)) {
            terminalUi->getBoardRenderer().render(board, players, turnManager);
        }
        return CommandResult(false, true);
    }

    player.setStatus(PlayerStatus::ACTIVE);
    player.setJailTurns(0);
    player.setConsecutiveDoubles(0);
    player.setHasRolledThisTurn(false);
    ui.showMessage("Double! Kamu bebas dari penjara.");
    ui.showMessage("Silakan lempar dadu lagi untuk melanjutkan giliran.");
    if (GameUI* terminalUi = asTerminalUi(ui)) {
        terminalUi->getBoardRenderer().render(board, players, turnManager);
    }
    return CommandResult(false, false);
}

CommandResult CommandProcessor::processDiceCommand(const Command& command, Player& player) {
    const std::string keyword = command.getKeyword();
    if (player.isJailed()) {
        return processJailDiceAttempt(command, player);
    }

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
    player.setActionTakenThisTurn(true);

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
            if (GameUI* terminalUi = asTerminalUi(ui)) {
                terminalUi->getBoardRenderer().render(board, players, turnManager);
            }
            return CommandResult(false, true);
        }
    } else {
        player.setConsecutiveDoubles(0);
    }

    if (logger != nullptr) {
        logger->log(
            turnManager.getCurrentTurn(),
            player.getUsername(),
            "DADU",
            "Lempar: " + std::to_string(dice.getDie1()) + "+" +
                std::to_string(dice.getDie2()) + "=" + std::to_string(dice.getTotal()));
    }

    ui.showDiceRoll(dice.getDie1(), dice.getDie2());
    resolveMovement(player, dice.getTotal());

    if (GameUI* terminalUi = asTerminalUi(ui)) {
        terminalUi->getBoardRenderer().render(board, players, turnManager);
    }
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
    std::vector<SkillCard*> usableCards;
    for (SkillCard* card : hand) {
        if (card != nullptr && (!player.isJailed() || card->canUseWhileJailed())) {
            usableCards.push_back(card);
        }
    }

    if (usableCards.empty()) {
        if (player.isJailed()) {
            ui.showMessage("Tidak ada kartu kemampuan yang dapat digunakan saat berada di penjara.");
        } else {
            ui.showMessage("Tidak ada kartu kemampuan.");
        }
        return;
    }

    int choice = ui.promptSkillCardSelection(
        "Pilih kartu kemampuan yang ingin digunakan",
        usableCards,
        true
    );
    if (choice == 0) {
        ui.showMessage("Penggunaan kartu dibatalkan.");
        return;
    }

    SkillCard* card = usableCards[choice - 1];
    card->use(player, *context);
    player.setActionTakenThisTurn(true);
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

    GameUI* terminalUi = asTerminalUi(ui);
    if (terminalUi == nullptr) {
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

    terminalUi->showLog(entries);
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
        if (GameUI* terminalUi = asTerminalUi(ui)) {
            terminalUi->showHelp(player);
        } else {
            ui.showMessage("Aksi tersedia: roll/set dice, build, mortgage, unmortgage, use skill, pay fine, save, dan lihat log.");
        }
        return CommandResult();
    }

    if (keyword == "KELUAR") {
        return CommandResult(true, false);
    }

    if (keyword == "CETAK_PAPAN") {
        if (GameUI* terminalUi = asTerminalUi(ui)) {
            terminalUi->getBoardRenderer().render(board, players, turnManager);
        }
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
        if (tile != nullptr) {
            property = tile->asPropertyTile();
        }
        if (GameUI* terminalUi = asTerminalUi(ui)) {
            terminalUi->getPropertyCardRenderer().renderDeed(property);
        }
        return CommandResult();
    }

    if (keyword == "CETAK_PROPERTI") {
        if (GameUI* terminalUi = asTerminalUi(ui)) {
            terminalUi->getPropertyCardRenderer().renderPlayerProperties(player);
        }
        return CommandResult();
    }

    if (keyword == "GADAI") {
        AssetManager::mortgageProperty(player, *context);
        player.setActionTakenThisTurn(true);
        return CommandResult();
    }

    if (keyword == "TEBUS" || keyword == "UNMORTGAGE") {
        AssetManager::redeemProperty(player, *context);
        player.setActionTakenThisTurn(true);
        return CommandResult();
    }

    if (keyword == "BANGUN") {
        AssetManager::buildProperty(player, *context);
        player.setActionTakenThisTurn(true);
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
        if (player.hasTakenActionThisTurn()) {
            throw InvalidCommandException(
                keyword,
                "SIMPAN hanya dapat dilakukan di awal giliran sebelum pemain menjalankan aksi apapun.");
        }

        const std::string filename = command.getArg(0);
        SaveManager saveManager;
        saveManager.saveGame(filename, createGameState());
        ui.showMessage("Permainan berhasil disimpan ke " + saveManager.getResolvedDataPath(filename));
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

    if (keyword == "BAYAR_DENDA") {
        bool wasJailed = player.isJailed();
        CommandResult result = payJailFine(player);
        if (wasJailed) {
            player.setActionTakenThisTurn(true);
        }
        return result;
    }

    throw InvalidCommandException(keyword, "command tidak dikenali");
}
