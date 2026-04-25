#pragma once

#include "SkillCard.hpp"

class MoveCard : public SkillCard {
public:
    MoveCard();
    explicit MoveCard(int value);

    std::string getTypeName() const override;
    bool canUseWhileJailed() const override;
    void use(Player& player, GameContext& gameContext) override;
};
