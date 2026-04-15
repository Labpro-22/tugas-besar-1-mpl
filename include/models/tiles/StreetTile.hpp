#pragma once
#include "PropertyTile.hpp"

class StreetTile: public PropertyTile{
private:
    ColorGroup colorGroup;
    int rentTable[6];
    int houseCost;
    int hotelCost;
    int buildingLevel;
    int festivalMultiplier;
    int festivalDuration;
public:
    void onLanded(Player& player, GameContext& gameContext) override;
    string getDisplayLabel() const override;

    int calculateRent(int diceTotal, GameContext& gameContext) override;

    void build();
    int sellBuilding();
    void applyFestival();
    void tickFestival();
    
    int getBuildingLevel() const;
    ColorGroup getColorGroup() const;
    bool canBuildNext(ColorGroup colorGroup);
    int getSellValueToBank() const;
    string getDisplayLabel() const override;
};