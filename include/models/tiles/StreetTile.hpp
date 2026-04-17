#pragma once

#include "PropertyTile.hpp"

class StreetTile : public PropertyTile {
private:
    ColorGroup colorGroup;
    int rentTable[6];
    int houseCost;
    int hotelCost;
    int buildingLevel;
    int festivalMultiplier;
    int festivalDuration;

public:
    StreetTile();
    StreetTile(
        int index,
        const std::string& code,
        const std::string& name,
        ColorGroup colorGroup,
        int buyPrice,
        int mortgageValue,
        int houseCost,
        int hotelCost,
        const int rentTable[6]
    );

    void onLanded(Player& player, GameContext& gameContext) override;
    int calculateRent(int diceTotal, const GameContext& gameContext) override;

    void build();
    int sellBuilding();
    void applyFestival();
    void tickFestival();

    int getBuildingLevel() const;
    ColorGroup getColorGroup() const;
    int getFestivalMultiplier() const;
    int getFestivalDuration() const;
    bool canBuildNext(ColorGroup colorGroup) const;
    int getSellValueToBank() const override;
    std::string getDisplayLabel() const override;
};
