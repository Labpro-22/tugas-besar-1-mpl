#pragma once

#include "SkillCard.hpp"

class DemolitionCard : public SkillCard {
public:
    DemolitionCard();

    std::string getTypeName() const override;
    void use(Player& player, GameContext& gameContext) override;
};

