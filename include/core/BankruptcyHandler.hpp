#pragma once

#include <vector>

class Player;
class AuctionManager;
class GameContext;
class PropertyTile;

class BankruptcyHandler {
public:
    BankruptcyHandler() = default;

    bool handleBankruptcy(Player& player, Player* creditor, int amount, GameContext& context);
    int calculateLiquidationMax(Player& player) const;
    bool liquidateAssets(Player& player, int amount, GameContext& context);
    void auctionBankruptAssets(const std::vector<PropertyTile*>& assets, GameContext& context);
    void transferAssetsToPlayer(Player& from, Player& to);
    void transferAssetsToBank(Player& player, AuctionManager* auction);
    void declareBankrupt(Player& player, GameContext& context);
};
