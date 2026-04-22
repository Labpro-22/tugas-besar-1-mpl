#include "models/cards/ShieldCard.hpp"

#include "core/GameContext.hpp"
#include "models/Player.hpp"

ShieldCard::ShieldCard()
    : SkillCard() {}

std::string ShieldCard::getTypeName() const {
    return "ShieldCard";
}

void ShieldCard::use(Player& player, GameContext& gameContext) {
    (void)gameContext;

    player.setShieldActive(true);
    player.setUsedSkillThisTurn(true);
}
