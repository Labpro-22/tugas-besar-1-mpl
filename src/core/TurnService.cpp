#include "core/TurnService.hpp"

#include <string>
#include <vector>

#include "core/Board.hpp"
#include "core/DeckFactory.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "models/cards/SkillCard.hpp"
#include "models/config/ConfigData.hpp"
#include "models/config/SpecialConfig.hpp"
#include "utils/CardDeck.hpp"
#include "utils/TransactionLogger.hpp"

namespace {
    void logTurnAction(
        TransactionLogger* logger,
        TurnManager& turnManager,
        const Player& player,
        const std::string& actionType,
        const std::string& detail
    ) {
        if (logger != nullptr) {
            logger->log(turnManager.getCurrentTurn(), player.getUsername(), actionType, detail);
        }
    }
}

void TurnService::processTurn(
    Player& player,
    Board& board,
    CardDeck<SkillCard>& skillDeck,
    const ConfigData& configData,
    GameIO& io,
    TurnManager& turnManager,
    TransactionLogger* logger
) {
    board.tickFestivals(player);
    player.resetTurnState();

    if (player.isJailed()) {
        player.setJailTurns(player.getJailTurns() + 1);
        int fine = configData.getSpecialConfig().getJailFine();
        io.showMessage(
            player.getUsername() + " sedang di penjara. Bayar denda M"
                + std::to_string(fine) + " untuk keluar.");

        if (player.canAfford(fine)) {
            player -= fine;
            player.setStatus(PlayerStatus::ACTIVE);
            player.setJailTurns(0);
            logTurnAction(
                logger,
                turnManager,
                player,
                "PENJARA",
                "Membayar denda M" + std::to_string(fine));
        }
    }

    try {
        SkillCard* card = skillDeck.draw();
        io.showMessage("Kamu mendapatkan 1 kartu acak baru!");
        io.showMessage("Kartu yang didapat: " + card->getTypeName() + ".");

        if (player.getHand().size() < 3) {
            player.addCard(card);
        } else {
            std::vector<SkillCard*> hand = player.getHand();
            io.showMessage("PERINGATAN: Kamu sudah memiliki 3 kartu di tangan (Maksimal 3)!");
            io.showMessage("Kamu diwajibkan membuang 1 kartu.");
            io.showMessage("Daftar Kartu Kemampuan Anda:");

            for (int i = 0; i < static_cast<int>(hand.size()); ++i) {
                io.showMessage(
                    std::to_string(i + 1) + ". " + hand[i]->getTypeName() +
                        " - " + DeckFactory::describeSkillCard(hand[i]));
            }
            io.showMessage(
                "4. " + card->getTypeName() + " - " + DeckFactory::describeSkillCard(card));

            int discardChoice = io.promptIntInRange("Pilih nomor kartu yang ingin dibuang (1-4): ", 1, 4);
            if (discardChoice == 4) {
                io.showMessage(card->getTypeName() + " telah dibuang. Sekarang kamu memiliki 3 kartu di tangan.");
                skillDeck.discardCard(card);
            } else {
                SkillCard* discarded = hand[discardChoice - 1];
                player.removeCard(discarded);
                skillDeck.discardCard(discarded);
                player.addCard(card);
                io.showMessage(
                    discarded->getTypeName() +
                        " telah dibuang. Sekarang kamu memiliki 3 kartu di tangan.");
            }
        }

        logTurnAction(
            logger,
            turnManager,
            player,
            "KARTU_SKILL",
            "Mendapatkan " + card->getTypeName());
    } catch (const std::exception&) {
    }
}
