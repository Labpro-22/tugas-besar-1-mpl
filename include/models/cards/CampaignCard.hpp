#pragma once

#include "ActionCard.hpp"

class CampaignCard : public ActionCard {
private:
    int amount;

public:
    CampaignCard();
    explicit CampaignCard(int amount);

    void execute(Player& player, GameContext& gameContext) override;
};
