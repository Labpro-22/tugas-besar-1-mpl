#pragma once

#include "SkillCard.hpp"

class ShieldCard : public SkillCard {
public:
    ShieldCard();

    std::string getTypeName() const override;
    void use(Player& player, GameContext& gameContext) override;
};


