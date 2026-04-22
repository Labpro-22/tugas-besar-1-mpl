#pragma once

class GameContext;
class Player;
class PropertyTile;

class PropertyTransactionService {
public:
    static void handlePropertyLanded(Player& player, PropertyTile& tile, GameContext& context);
    static void handleRentPayment(Player& player, PropertyTile& tile, GameContext& context);
};
