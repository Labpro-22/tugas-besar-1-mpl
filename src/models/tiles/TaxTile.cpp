#include "models/tiles/TaxTile.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "utils/TransactionLogger.hpp"
#include "core/TurnManager.hpp"
#include "core/GameIO.hpp"
#include "utils/TextFormatter.hpp"
#include "utils/exceptions/NimonspoliException.hpp"
#include "core/BankruptcyHandler.hpp"

TaxTile::TaxTile() : ActionTile(), taxType(TaxType::PPH), flatAmount(0), percentage(0) {}

TaxTile::TaxTile(int index, const std::string& code, const std::string& name, TaxType taxType, int flatAmount, int percentage)
    : ActionTile(index, code, name, TileCategory::DEFAULT), taxType(taxType), flatAmount(flatAmount), percentage(percentage) {}

void TaxTile::onLanded(Player& player, GameContext& gameContext) {
    int choice = 1;
    GameIO* io = gameContext.getIO();

    gameContext.showMessage("Kamu mendarat di " + getName() + " (" + getCode() + ")!");

    if (taxType == TaxType::PPH && io != nullptr) {
        if (io->usesRichGuiPresentation()) {
            const int wealth = calculateWealth(player);
            const int percentageAmount = (wealth * percentage) / 100;
            choice = io->promptTaxPaymentOption(
                player,
                getName(),
                flatAmount,
                percentage,
                wealth,
                percentageAmount);
        } else {
            gameContext.showMessage("Pilih opsi pembayaran pajak:");
            gameContext.showMessage("1. Bayar flat " + TextFormatter::formatMoney(flatAmount));
            gameContext.showMessage("2. Bayar " + std::to_string(percentage) + "% dari total kekayaan");
            gameContext.showMessage("(Pilih sebelum menghitung kekayaan!)");
            choice = gameContext.promptIntInRange("Pilihan (1/2): ", 1, 2);
        }
    }

    int amountToPay = calculateTaxAmount(player, choice);
    if (taxType == TaxType::PPH && choice == 2 && gameContext.hasIO()) {
        int cashWealth = player.getBalance();
        int propertyBuyWealth = 0;
        int buildingWealth = 0;

        for (PropertyTile* property : player.getProperties()) {
            if (property == nullptr) {
                continue;
            }

            propertyBuyWealth += property->getBuyPrice();

            if (property->getPropertyType() == PropertyType::STREET) {
                int level = property->getBuildingLevel();
                if (level > 0 && level <= 4) {
                    buildingWealth += level * property->getHouseCost();
                } else if (level == 5) {
                    buildingWealth += (4 * property->getHouseCost()) + property->getHotelCost();
                }
            }
        }

        gameContext.showMessage("");
        gameContext.showMessage("Total kekayaan kamu:");
        gameContext.showMessage("- Uang tunai          : " + TextFormatter::formatMoney(cashWealth));
        gameContext.showMessage("- Harga beli properti : " + TextFormatter::formatMoney(propertyBuyWealth) + " (termasuk yang digadaikan)");
        gameContext.showMessage("- Harga beli bangunan : " + TextFormatter::formatMoney(buildingWealth));
        gameContext.showMessage("Total                 : " + TextFormatter::formatMoney(calculateWealth(player)));
        gameContext.showMessage("Pajak " + std::to_string(percentage) + "%             : " + TextFormatter::formatMoney(amountToPay));
    } else if (taxType == TaxType::PBM && gameContext.hasIO()) {
        gameContext.showMessage("Pajak sebesar " + TextFormatter::formatMoney(amountToPay) + " langsung dipotong.");
    }

    applyTax(player, gameContext, amountToPay, choice);
}

void TaxTile::applyTax(Player& player, GameContext& gameContext, int amountToPay, int choice) const {
    if (player.isShieldActive()) {
        gameContext.showMessage("[SHIELD ACTIVE]: Efek ShieldCard melindungi Anda dari pajak.");
        gameContext.logEvent(
            "PAJAK",
            player.getUsername() + " terlindungi ShieldCard dari pajak.");
        return;
    }

    int originalAmount = amountToPay;
    amountToPay = player.consumeDiscountedAmount(amountToPay);
    if (amountToPay != originalAmount) {
        gameContext.showMessage(
            "Diskon diterapkan dari " + TextFormatter::formatMoney(originalAmount) +
                " menjadi " + TextFormatter::formatMoney(amountToPay) + ".");
    }

    try {
        int beforeBalance = player.getBalance();
        player -= amountToPay;
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showPaymentNotification(
                "PAYMENT",
                player.getUsername() + " membayar pajak " +
                    TextFormatter::formatMoney(amountToPay) + " ke Bank.");
        }
        gameContext.showMessage(
            "Uang kamu: " + TextFormatter::formatMoney(beforeBalance) +
                " -> " + TextFormatter::formatMoney(player.getBalance()));
        if (gameContext.getLogger() != nullptr) {
            int currentTurn = 0;
            if (gameContext.getTurnManager() != nullptr) {
                currentTurn = gameContext.getTurnManager()->getCurrentTurn();
            }
            gameContext.getLogger()->log(currentTurn, player.getUsername(), "PAJAK",
                "Membayar pajak sebesar " + TextFormatter::formatMoney(amountToPay));
        }
    } catch (const InsufficientFundsException& e) {
        if (taxType == TaxType::PPH && choice == 1) {
            gameContext.showMessage(
                "Kamu tidak mampu membayar pajak flat " + TextFormatter::formatMoney(amountToPay) + "!");
        } else {
            gameContext.showMessage("Kamu tidak mampu membayar pajak!");
        }
        gameContext.showMessage("Uang kamu saat ini: " + TextFormatter::formatMoney(player.getBalance()));
        if (gameContext.getBankruptcyHandler() != nullptr) {
            const bool settled = gameContext.getBankruptcyHandler()->handleBankruptcy(player, nullptr, amountToPay, gameContext);
            if (!settled && gameContext.getIO() != nullptr) {
                gameContext.getIO()->showMessage("Pembayaran pajak dibatalkan. Pajak belum dibayar.");
            }
        } else {
            throw;
        }
    }
}

int TaxTile::calculateTaxAmount(const Player& player, int choice) const {
    if (taxType == TaxType::PPH && choice == 2) {
        return (calculateWealth(player) * percentage) / 100;
    }
    return flatAmount;
}

int TaxTile::calculateWealth(const Player& player) const {
    int total = player.getBalance();
    
    for (auto* property : player.getProperties()) {
        if (property == nullptr) {
            continue;
        }

        total += property->getBuyPrice();
        
        if (property->getPropertyType() == PropertyType::STREET) {
            int level = property->getBuildingLevel();
            if (level > 0 && level <= 4) {
                total += level * property->getHouseCost();
            } else if (level == 5) {
                total += (4 * property->getHouseCost()) + property->getHotelCost();
            }
        }
    }
    return total;
}

TaxType TaxTile::getTaxType() const { return taxType; }
int TaxTile::getFlatAmount() const { return flatAmount; }
int TaxTile::getPercentage() const { return percentage; }
