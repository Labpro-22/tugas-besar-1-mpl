#include "core/CommandProcessor.hpp"

#include <stdexcept>
#include <string>

#include "core/AssetManager.hpp"
#include "core/BankruptcyHandler.hpp"
#include "core/Board.hpp"
#include "core/DeckFactory.hpp"
#include "core/Dice.hpp"
#include "core/GameContext.hpp"
#include "core/MovementService.hpp"
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
#include "utils/TextFormatter.hpp"
#include "utils/TransactionLogger.hpp"
#include "utils/exceptions/NimonspoliException.hpp"

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
    void expireTemporarySkillEffects(Player& player, GameIO& ui, const std::string& reason) {
        const bool hadShield = player.isShieldActive();
        const bool hadDiscount = player.getDiscountPercent() > 0;
        if (!hadShield && !hadDiscount) {
            return;
        }

        player.clearTemporarySkillEffects();
        if (hadShield) {
            ui.showMessage("Efek ShieldCard berakhir " + reason + ".");
        }
        if (hadDiscount) {
            ui.showMessage("Efek DiscountCard berakhir " + reason + ".");
        }
    }

    void applyJailState(Player& player, Tile* jailTile) {
        if (jailTile != nullptr) {
            jailTile->applyJailStatus(player);
            return;
        }

        player.setStatus(PlayerStatus::JAILED);
        player.setJailTurns(0);
        player.setConsecutiveDoubles(0);
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

int CommandProcessor::getJailFine() const {
    Tile* jailTile = board.getTile("PEN");
    if (jailTile == nullptr) {
        return 0;
    }
    return jailTile->getJailFine();
}

Tile* CommandProcessor::resolveMovement(Player& player, int totalMove) {
    int tileCount = board.getTileCount();
    if (tileCount <= 0) {
        throw GameInitException("papan permainan belum siap.");
    }

    const int oldPosition = player.getPosition();
    for (int step = 0; step < totalMove; ++step) {
        const int previousPosition = player.getPosition();
        const int nextPosition = (previousPosition + 1) % tileCount;
        player.moveTo(nextPosition);
        ui.showPawnStep(player, nextPosition);
    }

    Tile* landedTile = board.getTile(player.getPosition());
    if (logger != nullptr) {
        std::string detail =
            "Lempar: " + std::to_string(dice.getDie1()) + "+" +
            std::to_string(dice.getDie2()) + "=" + std::to_string(dice.getTotal());
        if (dice.isDouble()) {
            detail += " (double)";
        }
        if (landedTile != nullptr) {
            detail += " -> mendarat di " + landedTile->getName() +
                      " (" + landedTile->getCode() + ")";
        }

        logger->log(
            turnManager.getCurrentTurn(),
            player.getUsername(),
            "DADU",
            detail);
    }

    if (landedTile != nullptr && context != nullptr) {
        MovementService::awardGoSalaryForForwardMovement(
            board,
            player,
            *context,
            oldPosition,
            player.getPosition()
        );
        ui.showDiceLanding(
            dice.getDie1(),
            dice.getDie2(),
            dice.getTotal(),
            player.getUsername(),
            landedTile->getName(),
            landedTile->getCode());
        landedTile->onLanded(player, *context);
    }

    return landedTile;
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
            "Diskon diterapkan dari " + TextFormatter::formatMoney(fine) +
                " menjadi " + TextFormatter::formatMoney(fineToPay) + ".");
    }

    bool finePaid = false;
    if (fineToPay > 0 && !player.canAfford(fineToPay)) {
        ui.showMessage("Yahhhh.... uang kamu tidak cukup untuk membayar denda. Aset akan dilikuidasi bila memungkinkan.");
        if (context != nullptr && context->getBankruptcyHandler() != nullptr) {
            finePaid = context->getBankruptcyHandler()->handleBankruptcy(player, nullptr, fineToPay, *context) && !player.isBankrupt();
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
    ui.showPaymentNotification(
        "PAYMENT",
        player.getUsername() + " membayar denda " + TextFormatter::formatMoney(fineToPay) +
            " untuk keluar dari penjara.");
    ui.showMessage("Denda " + TextFormatter::formatMoney(fineToPay) + " dibayar. Kamu bebas dari penjara.");

    if (logger != nullptr) {
        logger->log(
            turnManager.getCurrentTurn(),
            player.getUsername(),
            "PENJARA",
            "Membayar denda " + TextFormatter::formatMoney(fineToPay) + " untuk keluar dari penjara.");
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
        throw InvalidCommandException(keyword, "Sipir penjara udah bosan lihat kamu, kamu wajib BAYAR_DENDA untuk keluar dari penjara.");
    }

    if (keyword == "ATUR_DADU") {
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
        ui.showMessage("Yahhhh.... busuk kali hoki u. Kamu masih berada di penjara dan giliran berakhir.");
        if (player.getJailTurns() >= 3) {
            ui.showMessage("Percobaan sudah 3 kali. Kamu harus BAYAR_DENDA yaaa...");
        }
        expireTemporarySkillEffects(player, ui, "saat giliran berakhir");
        ui.renderBoard(board, players, turnManager);
        return CommandResult(false, true);
    }

    player.setStatus(PlayerStatus::ACTIVE);
    player.setJailTurns(0);
    player.setConsecutiveDoubles(0);
    player.setHasRolledThisTurn(false);
    ui.showMessage("GACORRR DOUBLEEE! Kamu bebas dari penjara.");
    ui.showMessage("Silakan lempar dadu lagi untuk melanjutkan giliran.");
    ui.renderBoard(board, players, turnManager);
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
    player.setHasRolledMovementDiceThisTurn(true);
    player.setActionTakenThisTurn(true);

    const bool rolledDouble = dice.getDie1() == dice.getDie2();
    if (rolledDouble) {
        player.setConsecutiveDoubles(player.getConsecutiveDoubles() + 1);
        if (player.getConsecutiveDoubles() >= 3) {
            ui.showDiceRoll(dice.getDie1(), dice.getDie2());
            Tile* jailTile = board.getTile("PEN");
            applyJailState(player, jailTile);
            expireTemporarySkillEffects(player, ui, "saat giliran berakhir");
            ui.showMessage("Kamu mendapat tiga kali double berturut-turut!");
            ui.showMessage("Kamu nggak bisa hoki selamanya, Bung!");
            ui.showMessage("Bidak langsung dipindahkan ke Penjara dan giliran berakhir.");
            ui.showMessage("AOWKOWKOWK Anda masuk Penjara");
            if (logger != nullptr) {
                logger->log(
                    turnManager.getCurrentTurn(),
                    player.getUsername(),
                    "PENJARA",
                    "Masuk penjara karena tiga kali double.");
            }
            ui.renderBoard(board, players, turnManager);
            return CommandResult(false, true);
        }
    } else {
        player.setConsecutiveDoubles(0);
    }

    ui.showDiceRoll(dice.getDie1(), dice.getDie2());
    resolveMovement(player, dice.getTotal());
    expireTemporarySkillEffects(player, ui, "setelah gerakan pertama selesai");

    ui.renderBoard(board, players, turnManager);
    if (rolledDouble &&
        player.isActive() &&
        player.getConsecutiveDoubles() > 0 &&
        player.getConsecutiveDoubles() < 3) {
        ui.showMessage("BOOMMMM DOUBLEEEE! Kamu mendapat giliran tambahan.");
        if (logger != nullptr) {
            logger->log(
                turnManager.getCurrentTurn(),
                player.getUsername(),
                "DOUBLE",
                "Giliran tambahan ke-" + std::to_string(player.getConsecutiveDoubles()));
        }
        player.setHasRolledThisTurn(false);
        return CommandResult(false, false);
    }

    expireTemporarySkillEffects(player, ui, "saat giliran berakhir");
    return CommandResult(false, true);
}

void CommandProcessor::processSkillCommand(Player& player) {
    if (player.hasUsedSkillThisTurn()) {
        ui.showMessage("Kamu sudah menggunakan kartu kemampuan pada giliran ini!");
        ui.showMessage("Penggunaan kartu dibatasi maksimal 1 kali dalam 1 giliran. Jangan maruk yaa...");
        return;
    }

    if (player.hasRolledThisTurn() || player.hasRolledMovementDiceThisTurn()) {
        ui.showMessage("Kartu kemampuan hanya bisa digunakan SEBELUM melempar dadu.");
        return;
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
        "Daftar Kartu Kemampuan Spesial Anda:",
        usableCards,
        true
    );
    if (choice == 0) {
        ui.showMessage("Penggunaan kartu dibatalkan.");
        return;
    }

    SkillCard* card = usableCards[choice - 1];
    try {
        card->use(player, *context);
    } catch (const SkillUseFailedException&) {
        throw;
    }

    player.setUsedSkillThisTurn(true);
    player.setActionTakenThisTurn(true);
    player.removeCard(card);
    context->getSkillDeck()->discardCard(card);
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
        ui.showMessage("=== Log Transaksi (" + std::to_string(count) + " Terakhir) ===");
    } else {
        const std::vector<LogEntry>& all = logger->getAll();
        entries.assign(all.begin(), all.end());
        ui.showMessage("=== Log Transaksi Penuh ===");
    }

    ui.showMessage("");
    if (entries.empty()) {
        ui.showMessage("Belum ada log transaksi.");
        return;
    }
    ui.showLogEntries(entries);
}

CommandResult CommandProcessor::process(const Command& command, Player& player) {
    std::string keyword = command.getKeyword();
    if (keyword.empty()) {
        return CommandResult();
    }

    if (!command.isValid()) {
        if (!command.isKnown()) {
            throw InvalidCommandException(keyword, "command tidak dikenali");
        }
        const std::string usage = command.getUsage();
        throw InvalidCommandException(
            keyword,
            usage.empty() ? "format/argumen perintah tidak valid. Ketik HELP untuk bantuan."
                          : "format " + usage
        );
    }

    if (keyword == "HELP") {
        ui.showHelp(player);
        return CommandResult();
    }

    if (keyword == "KELUAR") {
        return CommandResult(true, false);
    }

    if (keyword == "CETAK_PAPAN") {
        ui.renderBoard(board, players, turnManager);
        return CommandResult();
    }

    if (keyword == "LEMPAR_DADU" || keyword == "ATUR_DADU") {
        return processDiceCommand(command, player);
    }

    if (keyword == "CETAK_AKTA") {
        const std::string code = command.getArg(0);
        Tile* tile = board.getTile(code);
        PropertyTile* property = nullptr;
        if (tile != nullptr) {
            property = tile->asPropertyTile();
        }
        if (property == nullptr) {
            ui.showMessage("Petak \"" + code + "\" tidak ditemukan atau bukan properti.");
            return CommandResult();
        }
        ui.showPropertyDeed(property);
        return CommandResult();
    }

    if (keyword == "CETAK_PROPERTI") {
        ui.showPlayerProperties(player);
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
        if (player.hasTakenActionThisTurn()) {
            throw InvalidCommandException(
                keyword,
                "SIMPAN hanya dapat dilakukan di awal giliran sebelum pemain menjalankan aksi apapun.");
        }

        const std::string filename = command.getArg(0);
        SaveManager saveManager;
        const std::string resolvedPath = saveManager.getResolvedDataPath(filename);
        const bool existedBefore = saveManager.fileExists(filename);
        ui.showMessage("Menyimpan permainan...");

        if (existedBefore) {
            ui.showMessage("File \"" + filename + "\" sudah ada.");
            if (!ui.confirmYN("Timpa file lama?")) {
                return CommandResult();
            }
        }

        try {
            if (logger != nullptr) {
                logger->log(
                    turnManager.getCurrentTurn(),
                    player.getUsername(),
                    "SAVE",
                    "Menyimpan permainan ke " + resolvedPath);
            }
            saveManager.saveGame(filename, createGameState());
        } catch (const SaveLoadException&) {
            ui.showMessage("Gagal menyimpan file! Pastikan direktori dapat ditulis.");
            return CommandResult();
        }

        if (existedBefore) {
            ui.showMessage("File berhasil ditimpa. Calon tukang timpa kamu, Bung :D");
        } else if (resolvedPath.find(filename) != std::string::npos) {
            ui.showMessage("Permainan berhasil disimpan ke: " + filename);
        } else {
            ui.showMessage("Permainan berhasil disimpan ke: " + resolvedPath);
        }
        return CommandResult();
    }

    if (keyword == "SURREND" || keyword == "SURRENDER") {
        ui.showMessage(player.getUsername() + " menyerah dan keluar dari permainan.");
        if (context != nullptr && context->getBankruptcyHandler() != nullptr) {
            context->getBankruptcyHandler()->declareBankrupt(player, *context);
        } else {
            player.setStatus(PlayerStatus::BANKRUPT);
        }
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
