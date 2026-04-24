#include "models/cards/CampaignCard.hpp"

#include "core/BankruptcyHandler.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "utils/OutputFormatter.hpp"

CampaignCard::CampaignCard()
    : CampaignCard(200) {}

CampaignCard::CampaignCard(int amount)
    : ActionCard("Anda mau nyaleg. Bayar " + OutputFormatter::formatMoney(amount) + " kepada setiap pemain."),
      amount(amount) {}

int CampaignCard::getAmount() const {
    return amount;
}

void CampaignCard::execute(Player& player, GameContext& gameContext) {
    if (player.isShieldActive()) {
        gameContext.showMessage(
            "[SHIELD ACTIVE]: Efek ShieldCard melindungi kamu dari tagihan CampaignCard.");
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
        if (otherPlayer == nullptr || otherPlayer == &player || otherPlayer->isBankrupt()) {
            continue;
        }

        int amountToPay = player.consumeDiscountedAmount(amount);
        if (amountToPay != amount) {
            gameContext.showMessage(
                "Diskon diterapkan dari " + OutputFormatter::formatMoney(amount) +
                    " menjadi " + OutputFormatter::formatMoney(amountToPay) + ".");
        }

        if (!player.canAfford(amountToPay)) {
            BankruptcyHandler* bankruptcyHandler = gameContext.getBankruptcyHandler();
            if (bankruptcyHandler != nullptr) {
                const bool settled = bankruptcyHandler->handleBankruptcy(player, otherPlayer, amountToPay, gameContext);
                if (!settled) {
                    if (gameContext.getIO() != nullptr) {
                        gameContext.getIO()->showMessage(
                            "Likuidasi dibatalkan. Pembayaran CampaignCard kepada " +
                            otherPlayer->getUsername() + " tidak jadi dilakukan.");
                    }
                    return;
                }
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
            gameContext.getIO()->showPaymentNotification(
                "PAYMENT",
                player.getUsername() + " membayar " +
                    OutputFormatter::formatMoney(amountToPay) + " kepada " +
                    otherPlayer->getUsername() + ".");
        }
        gameContext.showMessage(
            player.getUsername() + " membayar " + OutputFormatter::formatMoney(amountToPay) +
                " kepada " + otherPlayer->getUsername() + ".");
    }
}
