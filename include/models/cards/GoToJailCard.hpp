#pragma once

#include "models/cards/ActionCard.hpp"

class GoToJailCard : public ActionCard {
public:
    GoToJailCard();

    void execute(Player& player, GameContext& gameContext) override;
};


