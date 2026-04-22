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
    ColorGroup getColorGroup() const override;
    int getFestivalMultiplier() const override;
    int getFestivalDuration() const override;
    bool canBuildNext() const;
    int getSellValueToBank() const override;
    PropertyType getPropertyType() const override;
    std::string getDisplayLabel() const override;

    int getHouseCost() const override;
    int getHotelCost() const override;
    int getRentAtLevel(int level) const override;
    void setBuildingLevel(int level);
    void setFestivalState(int multiplier, int duration);
};
