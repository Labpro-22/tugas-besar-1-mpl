#include "models/cards/CampaignCard.hpp"

#include "core/BankruptcyHandler.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"

CampaignCard::CampaignCard()
    : CampaignCard(200) {}

CampaignCard::CampaignCard(int amount)
    : ActionCard("Anda mau nyaleg. Bayar M200 kepada setiap pemain."),
      amount(amount) {}

int CampaignCard::getAmount() const {
    return amount;
}

void CampaignCard::execute(Player& player, GameContext& gameContext) {
    if (player.isShieldActive()) {
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showMessage(
                "[SHIELD ACTIVE]: Efek ShieldCard melindungi kamu dari tagihan CampaignCard.");
        }
        gameContext.logEvent(
            "KARTU",
            player.getUsername() + " terlindungi ShieldCard dari CampaignCard.");
        return;
    }

    TurnManager* turnManager = gameContext.getTurnManager();
    if (turnManager == nullptr) {
        return;
    }

    std::vector<Player*> activePlayers = turnManager->getActivePlayers();
    for (Player* otherPlayer : activePlayers) {
        if (otherPlayer == nullptr || otherPlayer == &player || !otherPlayer->isActive()) {
            continue;
        }

        int amountToPay = player.consumeDiscountedAmount(amount);
        if (amountToPay != amount && gameContext.getIO() != nullptr) {
            gameContext.getIO()->showMessage(
                "Diskon diterapkan dari M" + std::to_string(amount) +
                    " menjadi M" + std::to_string(amountToPay) + ".");
        }

        if (!player.canAfford(amountToPay)) {
            BankruptcyHandler* bankruptcyHandler = gameContext.getBankruptcyHandler();
            if (bankruptcyHandler != nullptr) {
                bankruptcyHandler->handleBankruptcy(player, otherPlayer, amountToPay, gameContext);
            } else {
                player -= amountToPay;
                *otherPlayer += amountToPay;
            }
            if (player.isBankrupt()) {
                return;
            }
            continue;
        }

        player -= amountToPay;
        *otherPlayer += amountToPay;
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showMessage(
                player.getUsername() + " membayar M" + std::to_string(amountToPay) +
                    " kepada " + otherPlayer->getUsername() + ".");
        }
    }
}
