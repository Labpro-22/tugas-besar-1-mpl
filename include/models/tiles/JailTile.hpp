#pragma once
#include "ActionTile.hpp"

class JailTile: public ActionTile{
public:
    int jailFine;
private:
    void onLanded(Player& player, GameContext& gameContext) override;
    string getDisplayLabel() const override;
    void sendToJail(Player& player);
    void processJailTurn(Player& player, GameContext& gameContext);
};