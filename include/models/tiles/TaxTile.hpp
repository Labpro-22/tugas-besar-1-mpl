#pragma once
#include "ActionTile.hpp"

class FreeParkingTile: public ActionTile{
public:
    TaxType taxType;
    int flatAmount;
    int percentage;
private:
    void onLanded(Player& player, GameContext& gameContext) override;
    string getDisplayLabel() const override;
    int calculateWealth(Player& player) const;
};