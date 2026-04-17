#pragma once

class Player;
class Board;
class AuctionManager;
class GameContext;

class BankruptcyHandler {
public:
    BankruptcyHandler() = default;

    void handleBankruptcy(Player& player, Player* creditor, int amount, GameContext& context);
    int calculateLiquidationMax(Player& player) const;
    void showLiquidationPanel(Player& player, int needed, GameContext& context);
    void transferAssetsToPlayer(Player& from, Player& to);
    void transferAssetsToBank(Player& player, Board& board, AuctionManager& auction);
    void declareBankrupt(Player& player, GameContext& context);
};