#include "models/cards/BirthdayCard.hpp"

#include "core/BankruptcyHandler.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "utils/TextFormatter.hpp"
#include "utils/TransactionLogger.hpp"

namespace {
    void logBirthdayPayment(GameContext& gameContext, const Player& payer, const Player& recipient, int amount) {
        TransactionLogger* logger = gameContext.getLogger();
        TurnManager* turnManager = gameContext.getTurnManager();
        if (logger != nullptr && turnManager != nullptr) {
            logger->log(
                turnManager->getCurrentTurn(),
                payer.getUsername(),
                "KARTU",
                payer.getUsername() + " membayar BirthdayCard " +
                    TextFormatter::formatMoney(amount) + " kepada " + recipient.getUsername() + ".");
        }
    }
}

BirthdayCard::BirthdayCard()
    : BirthdayCard(100) {}

BirthdayCard::BirthdayCard(int amount)
    : ActionCard("SELAMAT ULANG TAHUNNN!!!. Dapatkan " + TextFormatter::formatMoney(amount) + " dari setiap pemain."),
      amount(amount) {}

void BirthdayCard::execute(Player& player, GameContext& gameContext) {
    TurnManager* turnManager = gameContext.getTurnManager();
    if (turnManager == nullptr) {
        return;
    }

    std::vector<Player*> activePlayers = turnManager->getActivePlayers();
    for (Player* otherPlayer : activePlayers) {
        if (otherPlayer == nullptr || otherPlayer == &player || otherPlayer->isBankrupt() || otherPlayer->isShieldActive()) {
            continue;
        }

        int amountToPay = otherPlayer->consumeDiscountedAmount(amount);
        if (amountToPay != amount) {
            gameContext.showMessage(
                "Diskon " + otherPlayer->getUsername() + " diterapkan dari " +
                TextFormatter::formatMoney(amount) + " menjadi " + TextFormatter::formatMoney(amountToPay) + ".");
        }

        if (!otherPlayer->canAfford(amountToPay)) {
            BankruptcyHandler* bankruptcyHandler = gameContext.getBankruptcyHandler();
            if (bankruptcyHandler != nullptr) {
                const bool settled = bankruptcyHandler->handleBankruptcy(*otherPlayer, &player, amountToPay, gameContext);
                if (!settled && gameContext.getIO() != nullptr) {
                    gameContext.getIO()->showMessage(
                        "Pembayaran ulang tahun dari " + otherPlayer->getUsername() +
                        " dibatalkan. Tidak ada transfer dana.");
                }
            } else {
                *otherPlayer -= amountToPay;
                player += amountToPay;
            }
            continue;
        }

        *otherPlayer -= amountToPay;
        player += amountToPay;
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showPaymentNotification(
                "PAYMENT",
                otherPlayer->getUsername() + " membayar " +
                    TextFormatter::formatMoney(amountToPay) + " kepada " +
                    player.getUsername() + ".");
        }
        gameContext.showMessage(
            otherPlayer->getUsername() + " memberi " + TextFormatter::formatMoney(amountToPay) +
                " kepada " + player.getUsername() + ".");
        logBirthdayPayment(gameContext, *otherPlayer, player, amountToPay);
    }
}
