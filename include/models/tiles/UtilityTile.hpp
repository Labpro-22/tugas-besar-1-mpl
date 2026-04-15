#pragma once
#include "PropertyTile.hpp"
#include <map>

class UtilityTile: public PropertyTile{
private:
    map<int, int> multiplierTable;
public:
    void onLanded(Player& player, GameContext& gameContext) override;
    int calculateRent(int diceTotal, GameContext& gameContext) override;
    int getSellValueToBank() const override;
    string getDisplayLabel() const override;
};