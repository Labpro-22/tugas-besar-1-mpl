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

        if (!player.canAfford(amount)) {
            gameContext.getBankruptcyHandler()->handleBankruptcy(player, otherPlayer, amount, gameContext);
            if (player.isBankrupt()) {
                return;
            }
            continue;
        }

        player -= amount;
        *otherPlayer += amount;
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showMessage(
                player.getUsername() + " membayar M" + std::to_string(amount) +
                    " kepada " + otherPlayer->getUsername() + ".");
        }
    }
}
