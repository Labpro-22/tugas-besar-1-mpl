#pragma once

#include "Tile.hpp"

class PropertyTile : public Tile {
private:
    Player* owner;
    PropertyStatus status;
    int buyPrice;
    int mortgageValue;

public:
    PropertyTile();
    PropertyTile(
        int index,
        const std::string& code,
        const std::string& name,
        TileCategory category,
        int buyPrice,
        int mortgageValue
    );
    virtual ~PropertyTile() = default;

    virtual int calculateRent(int diceTotal, const GameContext& gameContext) = 0;
    virtual int getSellValueToBank() const = 0;

    void mortgage();
    void redeem();
    void transferTo(Player& newOwner);
    void returnToBank();

    bool isOwnedBy(Player& player) const;
    bool isMortgaged() const;
    Player* getOwner() const;
    PropertyStatus getStatus() const;
    int getBuyPrice() const;
    int getMortgageValue() const;
};
