#include "models/cards/ShieldCard.hpp"

#include "core/GameContext.hpp"
#include "models/Player.hpp"

ShieldCard::ShieldCard()
    : SkillCard() {}

std::string ShieldCard::getTypeName() const {
    return "ShieldCard";
}

void ShieldCard::use(Player& player, GameContext& gameContext) {
    player.setShieldActive(true);
    gameContext.showMessage(
        "ShieldCard diaktifkan! Anda kebal terhadap tagihan atau sanksi selama giliran ini.");
    gameContext.logEvent("KARTU", player.getUsername() + " mengaktifkan ShieldCard.");
}
