#pragma once

class GameContext;
class Player;

class AssetManager {
public:
    static void mortgageProperty(Player& player, GameContext& context);
    static void redeemProperty(Player& player, GameContext& context);
    static void buildProperty(Player& player, GameContext& context);
};
