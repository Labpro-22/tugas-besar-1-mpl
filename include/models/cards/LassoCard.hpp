#pragma once

#include "SkillCard.hpp"

class LassoCard : public SkillCard {
public:
    LassoCard();

    std::string getTypeName() const override;
    void use(Player& player, GameContext& gameContext) override;
};
