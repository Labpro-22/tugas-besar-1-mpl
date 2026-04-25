#pragma once

#include "ActionCard.hpp"

class MoveBackCard : public ActionCard {
private:
    int steps;

public:
    MoveBackCard();
    explicit MoveBackCard(int steps);

    void execute(Player& player, GameContext& gameContext) override;
};
