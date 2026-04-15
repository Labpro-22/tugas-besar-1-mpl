#pragma once
#include "Tile.hpp"

class PropertyTile: public Tile{
private:
    Player* owner;
    PropertyStatus status;
    int buyPrice;
    int mortgageValue;
public:
    PropertyTile();
    PropertyTile(Player& owner, PropertyStatus status, int buyPrice, int mortgageValue, int index, string code, string name, TileCategory tileCategory);
    virtual ~PropertyTile() = default;

    virtual int calculateRent(int diceTotal, GameContext& gameContext) = 0;

    void mortgage() const;
    void redeem() const;
    void transferTo(Player player) const;
    void returnToBank();
    bool isOwnedBy(Player player);

    Player* getOwner() const;
    PropertyStatus getStatus() const;
    virtual int getSellValueToBank() const = 0;
    int getBuyPrice();
};
