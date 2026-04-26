#pragma once

#include "ActionTile.hpp"

class GoTile : public ActionTile {
private:
    int salary;

public:
    GoTile();
    GoTile(int index, const std::string& code, const std::string& name, int salary);

    void onLanded(Player& player, GameContext& gameContext) override;
    void onPassed(Player& player, GameContext& gameContext) override;
    void awardSalary(Player& player);
    int getSalary() const;
};
