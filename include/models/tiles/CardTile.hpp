#pragma once
#include "ActionTile.hpp"

class CardTile: public ActionTile{
public:
    CardType cardType;
private:
    void onLanded(Player& player, GameContext& gameContext) override;
    string getDisplayLabel() const override;
};