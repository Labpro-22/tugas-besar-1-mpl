#include "models/cards/CampaignCard.hpp"

#include "core/BankruptcyHandler.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/TurnManager.hpp"
#include "models/Player.hpp"
#include "utils/TextFormatter.hpp"

CampaignCard::CampaignCard()
    : CampaignCard(200) {}

CampaignCard::CampaignCard(int amount)
    : ActionCard("Anda mau nyaleg. Bayar " + TextFormatter::formatMoney(amount) + " kepada setiap pemain untuk kampanye."),
      amount(amount) {}

void CampaignCard::execute(Player& player, GameContext& gameContext) {
    if (player.isShieldActive()) {
        gameContext.showMessage(
            "[SHIELD ACTIVE]: Alhamdulillah, kamu berhasil melakukan kampanye gratis dengan ShieldCard!");
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
    std::vector<Player*> recipients;
    std::vector<int> payments;
    int totalAmountToPay = 0;

    for (Player* otherPlayer : activePlayers) {
        if (otherPlayer == nullptr || otherPlayer == &player || otherPlayer->isBankrupt()) {
            continue;
        }

        int amountToPay = player.consumeDiscountedAmount(amount);
        if (amountToPay != amount) {
            gameContext.showMessage(
                "Diskon diterapkan dari " + TextFormatter::formatMoney(amount) +
                    " menjadi " + TextFormatter::formatMoney(amountToPay) + ".");
        }

        recipients.push_back(otherPlayer);
        payments.push_back(amountToPay);
        totalAmountToPay += amountToPay;
    }

    if (recipients.empty()) {
        return;
    }

    if (!player.canAfford(totalAmountToPay)) {
        BankruptcyHandler* bankruptcyHandler = gameContext.getBankruptcyHandler();
        if (bankruptcyHandler != nullptr) {
            if (player.getLiquidationMax() < totalAmountToPay) {
                bankruptcyHandler->handleBankruptcy(player, nullptr, totalAmountToPay, gameContext);
                return;
            }

            gameContext.showMessage(
                "Total kewajiban CampaignCard: " + TextFormatter::formatMoney(totalAmountToPay) + ".");
            const bool settled = bankruptcyHandler->liquidateAssets(player, totalAmountToPay, gameContext);
            if (!settled || player.getBalance() < totalAmountToPay) {
                if (gameContext.getIO() != nullptr) {
                    gameContext.getIO()->showMessage(
                        "Likuidasi dibatalkan. Pembayaran CampaignCard tidak jadi dilakukan.");
                }
                return;
            }
        }
    }

    for (std::size_t index = 0; index < recipients.size(); ++index) {
        Player* otherPlayer = recipients[index];
        const int amountToPay = payments[index];

        player -= amountToPay;
        *otherPlayer += amountToPay;
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showPaymentNotification(
                "PAYMENT",
                player.getUsername() + " membayar " +
                    TextFormatter::formatMoney(amountToPay) + " kepada " +
                    otherPlayer->getUsername() + ".");
        }
        gameContext.showMessage(
            player.getUsername() + " membayar " + TextFormatter::formatMoney(amountToPay) +
                " kepada " + otherPlayer->getUsername() + ".");
        gameContext.logEvent(
            "KARTU",
            player.getUsername() + " membayar CampaignCard " +
                TextFormatter::formatMoney(amountToPay) + " kepada " +
                otherPlayer->getUsername() + ".");
    }
}
