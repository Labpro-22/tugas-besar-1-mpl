#include "models/cards/DoctorFeeCard.hpp"

#include <string>

#include "core/BankruptcyHandler.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "models/Player.hpp"
#include "utils/TextFormatter.hpp"

DoctorFeeCard::DoctorFeeCard()
    : DoctorFeeCard(700) {}

DoctorFeeCard::DoctorFeeCard(int amount)
    : ActionCard("Kamu sakit keras dan harus berobat! Bayar " + TextFormatter::formatMoney(amount) + " untuk Biaya Dokter."),
      amount(amount) {}

void DoctorFeeCard::execute(Player& player, GameContext& gameContext) {
    if (player.isShieldActive()) {
        gameContext.showMessage("[SHIELD ACTIVE]: Alhamdulillah, kamu dapat berobat gratis dengan ShieldCard!");
        gameContext.showMessage(
            "Tagihan " + TextFormatter::formatMoney(amount) +
                " dibatalkan. Uang Anda tetap: " + TextFormatter::formatMoney(player.getBalance()) + ".");
        gameContext.logEvent(
            "KARTU",
            player.getUsername() + " terlindungi ShieldCard dari DoctorFeeCard " +
                TextFormatter::formatMoney(amount) + ".");
        return;
    }

    int amountToPay = player.consumeDiscountedAmount(amount);
    if (amountToPay != amount) {
        gameContext.showMessage(
            "Diskon diterapkan dari " + TextFormatter::formatMoney(amount) +
                " menjadi " + TextFormatter::formatMoney(amountToPay) + ".");
    }

    if (player.canAfford(amountToPay)) {
        int beforeBalance = player.getBalance();
        player -= amountToPay;
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showPaymentNotification(
                "PAYMENT",
                player.getUsername() + " membayar biaya dokter " +
                    TextFormatter::formatMoney(amountToPay) + " ke Bank.");
        }
        gameContext.showMessage(
            "Kamu membayar " + TextFormatter::formatMoney(amountToPay) +
                " ke Bank. Sisa Uang = " + TextFormatter::formatMoney(player.getBalance()) + ".");
        gameContext.showMessage(
            "Uang kamu: " + TextFormatter::formatMoney(beforeBalance) +
                " -> " + TextFormatter::formatMoney(player.getBalance()) + ".");
        gameContext.logEvent(
            "KARTU",
            player.getUsername() + " membayar DoctorFeeCard " +
                TextFormatter::formatMoney(amountToPay) + " ke Bank.");
        return;
    }

    gameContext.showMessage(
        "Kamu tidak mampu membayar biaya dokter! (" + TextFormatter::formatMoney(amountToPay) + ")");
    gameContext.showMessage("Uang kamu saat ini: " + TextFormatter::formatMoney(player.getBalance()));

    BankruptcyHandler* bankruptcyHandler = gameContext.getBankruptcyHandler();
    if (bankruptcyHandler != nullptr) {
        const bool settled = bankruptcyHandler->handleBankruptcy(player, nullptr, amountToPay, gameContext);
        if (!settled && gameContext.getIO() != nullptr) {
            gameContext.getIO()->showMessage("Pembayaran biaya dokter dibatalkan. Tagihan belum terselesaikan.");
        }
        return;
    }

    player -= amountToPay;
}
