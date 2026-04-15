#pragma once
#include "ActionTile.hpp"

class GoTile: public ActionTile{
public:
    int salary;
private:
    void onLanded(Player& player, GameContext& gameContext) override;
    string getDisplayLabel() const override;
    void awardSalary(Player& player);
};