#include "models/cards/DiscountCard.hpp"

#include "core/GameContext.hpp"
#include "models/Player.hpp"

DiscountCard::DiscountCard()
    : SkillCard(0) {}

DiscountCard::DiscountCard(int value)
    : SkillCard(value) {}

std::string DiscountCard::getTypeName() const {
    return "DiscountCard";
}

void DiscountCard::use(Player& player, GameContext& gameContext) {
    player.setDiscountPercent(getValue());
    gameContext.showMessage(
        "DiscountCard diaktifkan! Pembayaran berikutnya mendapat diskon " +
            std::to_string(getValue()) + "%.");
    gameContext.logEvent(
        "KARTU",
        player.getUsername() + " mengaktifkan DiscountCard diskon " + std::to_string(getValue()) +
            "% untuk satu pembayaran berikutnya."
    );
}
