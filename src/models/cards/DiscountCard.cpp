#include "models/cards/DiscountCard.hpp"

#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
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
    setRemainingDuration(1);
    if (gameContext.getIO() != nullptr) {
        gameContext.getIO()->showMessage(
            "DiscountCard diaktifkan! Pembayaran berikutnya mendapat diskon " +
                std::to_string(getValue()) + "%.");
    }
    gameContext.logEvent(
        "KARTU",
        player.getUsername() + " mengaktifkan DiscountCard diskon " + std::to_string(getValue()) + "%."
    );
}
