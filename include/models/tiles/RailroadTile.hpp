#pragma once
#include <map>
#include "PropertyTile.hpp"

class RailRoadTile: public PropertyTile{
private:
    map<int, int> rentTable;
public:
    void onLanded(Player& player, GameContext& gameContext) override;
    int calculateRent(int diceTotal, GameContext& gameContext) override;
    int getSellValueToBank() const override;
    string getDisplayLabel() const override;
};