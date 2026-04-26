#pragma once

#include "SkillCard.hpp"

class DiscountCard : public SkillCard {
public:
    DiscountCard();
    explicit DiscountCard(int value);

    std::string getTypeName() const override;
    void use(Player& player, GameContext& gameContext) override;
};
