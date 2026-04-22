#include "models/cards/DoctorFeeCard.hpp"

#include "core/BankruptcyHandler.hpp"
#include "core/GameContext.hpp"
#include "models/Player.hpp"

DoctorFeeCard::DoctorFeeCard()
    : DoctorFeeCard(700) {}

DoctorFeeCard::DoctorFeeCard(int amount)
    : ActionCard("Biaya dokter. Anda harus bayar M700."),
      amount(amount) {}

int DoctorFeeCard::getAmount() const {
    return amount;
}

void DoctorFeeCard::execute(Player& player, GameContext& gameContext) {
    if (player.isShieldActive()) {
        return;
    }

    if (player.canAfford(amount)) {
        player -= amount;
        return;
    }

    BankruptcyHandler* bankruptcyHandler = gameContext.getBankruptcyHandler();
    if (bankruptcyHandler != nullptr) {
        bankruptcyHandler->handleBankruptcy(player, nullptr, amount, gameContext);
        return;
    }

    player -= amount;
}
