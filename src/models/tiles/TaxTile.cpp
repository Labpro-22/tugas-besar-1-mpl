#include "models/tiles/TaxTile.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "core/GameIO.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "utils/TransactionLogger.hpp"
#include "core/TurnManager.hpp"
#include "utils/exceptions/NimonspoliException.hpp"
#include "core/BankruptcyHandler.hpp"

TaxTile::TaxTile() : ActionTile(), taxType(TaxType::PPH), flatAmount(0), percentage(0) {}

TaxTile::TaxTile(int index, const std::string& code, const std::string& name, TaxType taxType, int flatAmount, int percentage)
    : ActionTile(index, code, name, TileCategory::DEFAULT), taxType(taxType), flatAmount(flatAmount), percentage(percentage) {}

void TaxTile::onLanded(Player& player, GameContext& gameContext) {
    GameIO* io = gameContext.getIO();
    int choice = 1;

    if (io != nullptr) {
        io->showMessage("Kamu mendarat di " + getName() + " (" + getCode() + ")!");
    }

    if (taxType == TaxType::PPH && io != nullptr) {
        io->showMessage("Pilih opsi pembayaran pajak:");
        io->showMessage("1. Bayar flat M" + std::to_string(flatAmount));
        io->showMessage("2. Bayar " + std::to_string(percentage) + "% dari total kekayaan");
        io->showMessage("(Pilih sebelum menghitung kekayaan!)");
        choice = io->promptIntInRange("Pilihan (1/2): ", 1, 2);
    }

    int amountToPay = calculateTaxAmount(player, choice);
    if (taxType == TaxType::PPH && choice == 2 && io != nullptr) {
        io->showMessage("Total kekayaan kamu: M" + std::to_string(calculateWealth(player)));
        io->showMessage(
            "Pajak " + std::to_string(percentage) + "%: M" + std::to_string(amountToPay));
    } else if (io != nullptr) {
        io->showMessage("Pajak yang harus dibayar: M" + std::to_string(amountToPay));
    }

    applyTax(player, gameContext, amountToPay);
}

void TaxTile::applyTax(Player& player, GameContext& gameContext, int amountToPay) const {
    if (player.isShieldActive()) {
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showMessage("[SHIELD ACTIVE]: Efek ShieldCard melindungi Anda dari pajak.");
        }
        gameContext.logEvent(
            "PAJAK",
            player.getUsername() + " terlindungi ShieldCard dari pajak.");
        return;
    }

    int originalAmount = amountToPay;
    amountToPay = player.consumeDiscountedAmount(amountToPay);
    if (amountToPay != originalAmount && gameContext.getIO() != nullptr) {
        gameContext.getIO()->showMessage(
            "Diskon diterapkan dari M" + std::to_string(originalAmount) +
                " menjadi M" + std::to_string(amountToPay) + ".");
    }

    try {
        int beforeBalance = player.getBalance();
        player -= amountToPay;
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showMessage(
                "Uang kamu: M" + std::to_string(beforeBalance) +
                    " -> M" + std::to_string(player.getBalance()));
        }
        if (gameContext.getLogger() != nullptr) {
            int currentTurn = 0;
            if (gameContext.getTurnManager() != nullptr) {
                currentTurn = gameContext.getTurnManager()->getCurrentTurn();
            }
            gameContext.getLogger()->log(currentTurn, player.getUsername(), "PAJAK",
                "Membayar pajak sebesar M" + std::to_string(amountToPay));
        }
    } catch (const InsufficientFundsException& e) {
        if (gameContext.getIO() != nullptr) {
            gameContext.getIO()->showMessage(
                "Kamu tidak mampu membayar pajak M" + std::to_string(amountToPay) + "!");
            gameContext.getIO()->showMessage("Uang kamu saat ini: M" + std::to_string(player.getBalance()));
        }
        if (gameContext.getBankruptcyHandler() != nullptr) {
            gameContext.getBankruptcyHandler()->handleBankruptcy(player, nullptr, amountToPay, gameContext);
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

std::string TaxTile::getDisplayLabel() const { return getCode(); }
TaxType TaxTile::getTaxType() const { return taxType; }
int TaxTile::getFlatAmount() const { return flatAmount; }
int TaxTile::getPercentage() const { return percentage; }
