#include "models/cards/DoctorFeeCard.hpp"

#include "core/BankruptcyHandler.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
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
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showMessage("[SHIELD ACTIVE]: Efek ShieldCard melindungi Anda!");
            gameContext.getIO()->showMessage(
                "Tagihan M" + std::to_string(amount) +
                    " dibatalkan. Uang Anda tetap: M" + std::to_string(player.getBalance()) + ".");
        }
        return;
    }

    if (player.canAfford(amount)) {
        int beforeBalance = player.getBalance();
        player -= amount;
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showPaymentNotification(
                "PAYMENT",
                player.getUsername() + " membayar biaya dokter M" +
                    std::to_string(amount) + " ke Bank.");
            gameContext.getIO()->showMessage(
                "Kamu membayar M" + std::to_string(amount) +
                    " ke Bank. Sisa Uang = M" + std::to_string(player.getBalance()) + ".");
            gameContext.getIO()->showMessage(
                "Uang kamu: M" + std::to_string(beforeBalance) +
                    " -> M" + std::to_string(player.getBalance()));
        }
        return;
    }

    if (gameContext.getIO() != nullptr) {
        gameContext.getIO()->showMessage(
            "Kamu tidak mampu membayar biaya dokter! (M" + std::to_string(amount) + ")");
        gameContext.getIO()->showMessage("Uang kamu saat ini: M" + std::to_string(player.getBalance()));
    }

    BankruptcyHandler* bankruptcyHandler = gameContext.getBankruptcyHandler();
    if (bankruptcyHandler != nullptr) {
        bankruptcyHandler->handleBankruptcy(player, nullptr, amount, gameContext);
        return;
    }

    player -= amount;
}
