#pragma once

#include <map>

#include "PropertyTile.hpp"

class RailroadTile : public PropertyTile {
private:
    std::map<int, int> rentTable;

public:
    RailroadTile();
    RailroadTile(
        int index,
        const std::string& code,
        const std::string& name,
        int mortgageValue,
        const std::map<int, int>& rentTable
    );

    void onLanded(Player& player, GameContext& gameContext) override;
    int calculateRent(int diceTotal, const GameContext& gameContext) override;
    RailroadTile* asRailroadTile() override;
    const RailroadTile* asRailroadTile() const override;
    int getSellValueToBank() const override;
    PropertyType getPropertyType() const override;
};
