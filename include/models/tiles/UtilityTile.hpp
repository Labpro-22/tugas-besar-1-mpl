#pragma once

#include <map>

#include "PropertyTile.hpp"

class UtilityTile : public PropertyTile {
private:
    std::map<int, int> multiplierTable;

public:
    UtilityTile();
    UtilityTile(
        int index,
        const std::string& code,
        const std::string& name,
        int mortgageValue,
        const std::map<int, int>& multiplierTable
    );

    void onLanded(Player& player, GameContext& gameContext) override;
    int calculateRent(int diceTotal, const GameContext& gameContext) override;
    int getSellValueToBank() const override;
    PropertyType getPropertyType() const override;
    std::string getDisplayLabel() const override;
};
