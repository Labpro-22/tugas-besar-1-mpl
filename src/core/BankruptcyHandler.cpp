#include "core/BankruptcyHandler.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "core/Board.hpp"
#include "core/AuctionManager.hpp"
#include "core/TurnManager.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include <string>
#include <vector>

void BankruptcyHandler::handleBankruptcy(Player& player, Player* creditor, int amount, GameContext& context) {
    int totalAvailable = player.getBalance() + calculateLiquidationMax(player);
    
    if (totalAvailable < amount) {
        if (creditor) {
            int transferred = player.getBalance();
            creditor->setBalance(creditor->getBalance() + player.getBalance());
            transferAssetsToPlayer(player, *creditor);
            if (context.getIO() != nullptr && transferred > 0) {
                context.getIO()->showPaymentNotification(
                    "PAYMENT",
                    player.getUsername() + " menyerahkan saldo terakhir M" +
                        std::to_string(transferred) + " kepada " + creditor->getUsername() + ".");
            }
        } else {
            transferAssetsToBank(player, context.getAuctionManager());
        }
        player.setBalance(0);
        declareBankrupt(player, context);
    } else {
        int needed = amount - player.getBalance();
        if (needed > 0) {
            showLiquidationPanel(player, needed, context);
        }
        player.setBalance(player.getBalance() - amount);
        if (creditor) {
            creditor->setBalance(creditor->getBalance() + amount);
        }
        if (context.getIO() != nullptr) {
            context.getIO()->showPaymentNotification(
                "PAYMENT",
                player.getUsername() + " membayar M" + std::to_string(amount) +
                    (creditor == nullptr ? " ke Bank." : " kepada " + creditor->getUsername() + "."));
        }
    }
}

int BankruptcyHandler::calculateLiquidationMax(Player& player) const {
    return player.getLiquidationMax();
}

void BankruptcyHandler::showLiquidationPanel(Player& player, int needed, GameContext& context) {
    std::vector<PropertyTile*> properties = player.getProperties();
    for (PropertyTile* property : properties) {
        if (player.getBalance() >= needed || property == nullptr) {
            break;
        }

        StreetTile* street = nullptr;
        street = property->asStreetTile();

        while (street != nullptr && street->getBuildingLevel() > 0 && player.getBalance() < needed) {
            int received = street->sellBuilding();
            player += received;
            context.logEvent(
                "LIKUIDASI",
                player.getUsername() + " menjual bangunan di " + street->getCode() + " dan menerima M" + std::to_string(received)
            );
        }

        if (property->getStatus() == PropertyStatus::OWNED && player.getBalance() < needed) {
            property->mortgage();
            player += property->getMortgageValue();
            context.logEvent(
                "LIKUIDASI",
                player.getUsername() + " menggadaikan " + property->getCode() + " dan menerima M" + std::to_string(property->getMortgageValue())
            );
        }
    }
}

void BankruptcyHandler::transferAssetsToPlayer(Player& from, Player& to) {
    std::vector<PropertyTile*> properties = from.getProperties();
    for (PropertyTile* prop : properties) {
        from.removeProperty(prop);
        to.addProperty(prop);
        prop->transferTo(to);
    }
}

void BankruptcyHandler::transferAssetsToBank(Player& player, AuctionManager* auction) {
    if (auction != nullptr) {
        auction->reset();
    }
    std::vector<PropertyTile*> properties = player.getProperties();
    for (PropertyTile* prop : properties) {
        if (prop != nullptr) {
            prop->returnToBank();
        }
    }
}

void BankruptcyHandler::declareBankrupt(Player& player, GameContext& context) {
    player.setStatus(PlayerStatus::BANKRUPT);
    if (context.getTurnManager() != nullptr) {
        context.getTurnManager()->removePlayer(&player);
    }
}
