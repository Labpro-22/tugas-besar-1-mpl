#include "models/cards/DoctorFeeCard.hpp"

#include <string>

#include "core/BankruptcyHandler.hpp"
#include "core/GameContext.hpp"
#include "models/Player.hpp"
#include "utils/OutputFormatter.hpp"

DoctorFeeCard::DoctorFeeCard()
    : DoctorFeeCard(700) {}

DoctorFeeCard::DoctorFeeCard(int amount)
    : ActionCard("Biaya dokter. Bayar " + OutputFormatter::formatMoney(amount) + "."),
      amount(amount) {}

int DoctorFeeCard::getAmount() const {
    return amount;
}

void DoctorFeeCard::execute(Player& player, GameContext& gameContext) {
    if (player.isShieldActive()) {
        gameContext.showMessage("[SHIELD ACTIVE]: Efek ShieldCard melindungi Anda!");
        gameContext.showMessage(
            "Tagihan M" + std::to_string(amount) +
                " dibatalkan. Uang Anda tetap: M" + std::to_string(player.getBalance()) + ".");
        return;
    }

    int amountToPay = player.consumeDiscountedAmount(amount);
    if (amountToPay != amount) {
        gameContext.showMessage(
            "Diskon diterapkan dari " + OutputFormatter::formatMoney(amount) +
                " menjadi " + OutputFormatter::formatMoney(amountToPay) + ".");
    }

    if (player.canAfford(amountToPay)) {
        player -= amountToPay;
        gameContext.showMessage(
            "Kamu membayar " + OutputFormatter::formatMoney(amountToPay) +
                " ke Bank. Sisa Uang = " + OutputFormatter::formatMoney(player.getBalance()) + ".");
        return;
    }

    gameContext.showMessage(
        "Kamu tidak mampu membayar biaya dokter! (" + OutputFormatter::formatMoney(amountToPay) + ")");
    gameContext.showMessage("Uang kamu saat ini: " + OutputFormatter::formatMoney(player.getBalance()));

    BankruptcyHandler* bankruptcyHandler = gameContext.getBankruptcyHandler();
    if (bankruptcyHandler != nullptr) {
        bankruptcyHandler->handleBankruptcy(player, nullptr, amountToPay, gameContext);
        return;
    }

    player -= amountToPay;
}
