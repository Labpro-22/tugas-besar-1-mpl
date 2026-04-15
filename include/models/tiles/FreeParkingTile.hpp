#pragma once
#include "ActionTile.hpp"

class FreeParkingTile: public ActionTile{
public:

private:
    void onLanded(Player& player, GameContext& gameContext) override;
    string getDisplayLabel() const override;
};