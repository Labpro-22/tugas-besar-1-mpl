#pragma once

#include "SkillCard.hpp"

class TeleportCard : public SkillCard {
public:
    TeleportCard();

    std::string getTypeName() const override;
    void use(Player& player, GameContext& gameContext) override;
};

