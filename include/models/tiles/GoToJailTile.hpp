#pragma once
#include "ActionTile.hpp"

class GoToJailTile: public ActionTile{
public:

private:
    void onLanded(Player& player, GameContext& gameContext) override;
    string getDisplayLabel() const override;
};