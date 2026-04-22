#pragma once

#include "SkillCard.hpp"

class MoveCard : public SkillCard {
public:
    MoveCard();
    MoveCard(int value, int remainingDuration);

    std::string getTypeName() const override;
    void use(Player& player, GameContext& gameContext) override;
};
