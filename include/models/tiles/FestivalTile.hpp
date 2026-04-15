#pragma once
#include "ActionTile.hpp"

class FestivalTile: public ActionTile{
public:

private:
    void onLanded(Player& player, GameContext& gameContext) override;
    string getDisplayLabel() const override;
};