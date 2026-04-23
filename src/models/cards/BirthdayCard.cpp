#include "models/cards/BirthdayCard.hpp"

#include "core/BankruptcyHandler.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"

BirthdayCard::BirthdayCard()
    : BirthdayCard(100) {}

BirthdayCard::BirthdayCard(int amount)
    : ActionCard("Ini adalah hari ulang tahun Anda. Dapatkan M100 dari setiap pemain."),
      amount(amount) {}

int BirthdayCard::getAmount() const {
    return amount;
}

void BirthdayCard::execute(Player& player, GameContext& gameContext) {
    TurnManager* turnManager = gameContext.getTurnManager();
    if (turnManager == nullptr) {
        return;
    }

    std::vector<Player*> activePlayers = turnManager->getActivePlayers();
    for (Player* otherPlayer : activePlayers) {
        if (otherPlayer == nullptr || otherPlayer == &player || !otherPlayer->isActive() || otherPlayer->isShieldActive()) {
            continue;
        }

        int amountToPay = otherPlayer->consumeDiscountedAmount(amount);
        if (amountToPay != amount && gameContext.getIO() != nullptr) {
            gameContext.getIO()->showMessage(
                "Diskon " + otherPlayer->getUsername() + " diterapkan dari M" +
                std::to_string(amount) + " menjadi M" + std::to_string(amountToPay) + ".");
        }

        if (!otherPlayer->canAfford(amountToPay)) {
            BankruptcyHandler* bankruptcyHandler = gameContext.getBankruptcyHandler();
            if (bankruptcyHandler != nullptr) {
                bankruptcyHandler->handleBankruptcy(*otherPlayer, &player, amountToPay, gameContext);
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
                otherPlayer->getUsername() + " membayar M" + std::to_string(amount) +
                    " kepada " + player.getUsername() + ".");
            gameContext.getIO()->showMessage(
                otherPlayer->getUsername() + " memberi M" + std::to_string(amountToPay) +
                    " kepada " + player.getUsername() + ".");
        }
    }
}
