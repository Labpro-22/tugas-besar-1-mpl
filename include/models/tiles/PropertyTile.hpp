#pragma once

#include "Tile.hpp"

class PropertyTile : public Tile {
private:
    Player* owner;
    PropertyStatus status;
    int mortgageValue;
    int buyPrice;
public:
    PropertyTile();
    PropertyTile(
        int index,
        const std::string& code,
        const std::string& name,
        int buyPrice,
        int mortgageValue
    );
    virtual ~PropertyTile() = default;

    virtual int calculateRent(int diceTotal, const GameContext& gameContext) = 0;
    virtual int getSellValueToBank() const = 0;
    virtual PropertyType getPropertyType() const = 0;

    void mortgage();
    void redeem();
    void transferTo(Player& newOwner);
    void returnToBank();

    bool isOwnedBy(const Player& player) const;
    bool isMortgaged() const;
    Player* getOwner() const;
    PropertyStatus getStatus() const;
    int getMortgageValue() const;
    int getBuyPrice() const;
    virtual ColorGroup getColorGroup() const;
    virtual int getBuildingLevel() const;
    virtual int getFestivalMultiplier() const;
    virtual int getFestivalDuration() const;
    virtual int getHouseCost() const;
    virtual int getHotelCost() const;
    virtual int getRentAtLevel(int level) const;
};
