#pragma once

#include "ActionCard.hpp"

class MoveToNearestStationCard : public ActionCard {
public:
    MoveToNearestStationCard();

    void execute(Player& player, GameContext& gameContext) override;
};


