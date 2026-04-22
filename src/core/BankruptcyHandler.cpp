#include "core/BankruptcyHandler.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "core/Board.hpp"
#include "core/AuctionManager.hpp"
#include "core/TurnManager.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include <vector>

void BankruptcyHandler::handleBankruptcy(Player& player, Player* creditor, int amount, GameContext& context) {
    int totalAvailable = player.getBalance() + calculateLiquidationMax(player);
    
    if (totalAvailable < amount) {
        if (creditor) {
            creditor->setBalance(creditor->getBalance() + player.getBalance());
            transferAssetsToPlayer(player, *creditor);
        } else {
            transferAssetsToBank(player, *context.getBoard(), *context.getAuctionManager());
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

        StreetTile* street = dynamic_cast<StreetTile*>(property);
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

void BankruptcyHandler::transferAssetsToBank(Player& player, Board&, AuctionManager& auction) {
    auction.reset();
    std::vector<PropertyTile*> properties = player.getProperties();
    for (PropertyTile* prop : properties) {
        prop->returnToBank();
    }
}

void BankruptcyHandler::declareBankrupt(Player& player, GameContext& context) {
    player.setStatus(PlayerStatus::BANKRUPT);
    if (context.getTurnManager() != nullptr) {
        context.getTurnManager()->removePlayer(&player);
    }
}
