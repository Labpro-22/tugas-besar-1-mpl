#include "models/cards/DiscountCard.hpp"

#include "core/GameContext.hpp"
#include "models/Player.hpp"

DiscountCard::DiscountCard()
    : SkillCard(0, 1) {}

DiscountCard::DiscountCard(int value, int remainingDuration)
    : SkillCard(value, remainingDuration) {}

std::string DiscountCard::getTypeName() const {
    return "DiscountCard";
}

void DiscountCard::use(Player& player, GameContext& gameContext) {
    player.setDiscountPercent(getValue());
    player.setUsedSkillThisTurn(true);
    setRemainingDuration(1);
    gameContext.logEvent(
        "KARTU",
        player.getUsername() + " mengaktifkan DiscountCard diskon " + std::to_string(getValue()) + "%."
    );
}
