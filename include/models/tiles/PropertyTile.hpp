#pragma once

#include "models/Enums.hpp"
#include "Tile.hpp"

class RailroadTile;
class StreetTile;

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
    PropertyTile* asPropertyTile() override;
    const PropertyTile* asPropertyTile() const override;
    virtual StreetTile* asStreetTile();
    virtual const StreetTile* asStreetTile() const;
    virtual RailroadTile* asRailroadTile();
    virtual const RailroadTile* asRailroadTile() const;

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
    virtual bool canBuildNext() const;
    virtual int sellBuilding();
    virtual void setBuildingLevel(int level);
    virtual void setFestivalState(int multiplier, int duration);
};
