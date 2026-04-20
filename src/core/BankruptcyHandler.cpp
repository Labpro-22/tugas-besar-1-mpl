#include "core/BankruptcyHandler.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "core/Board.hpp"
#include "core/AuctionManager.hpp"
#include "models/tiles/PropertyTile.hpp"
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
    
}

void BankruptcyHandler::transferAssetsToPlayer(Player& from, Player& to) {
    std::vector<PropertyTile*> properties = from.getProperties();
    for (PropertyTile* prop : properties) {
        from.removeProperty(prop);
        to.addProperty(prop);
        prop->transferTo(to);
    }
}

void BankruptcyHandler::transferAssetsToBank(Player& player, Board& board, AuctionManager& auction) {
    std::vector<PropertyTile*> properties = player.getProperties();
    for (PropertyTile* prop : properties) {
        player.removeProperty(prop);
        prop->returnToBank();
    }
}

void BankruptcyHandler::declareBankrupt(Player& player, GameContext& context) {
    player.setStatus(PlayerStatus::BANKRUPT);
}

