#pragma once

#include "SkillCard.hpp"

class DiscountCard : public SkillCard {
public:
    DiscountCard();
    DiscountCard(int value, int remainingDuration);

    std::string getTypeName() const override;
    void use(Player& player, GameContext& gameContext) override;
};
