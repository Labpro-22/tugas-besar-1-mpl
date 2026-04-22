#include "models/tiles/TaxTile.hpp"
#include "models/Player.hpp"
#include "core/GameContext.hpp"
#include "models/tiles/PropertyTile.hpp"
#include "models/tiles/StreetTile.hpp"
#include "utils/TransactionLogger.hpp"
#include "core/TurnManager.hpp"
#include "utils/exceptions/InsufficientFundsException.hpp"
#include "core/BankruptcyHandler.hpp"

TaxTile::TaxTile() : ActionTile(), taxType(TaxType::PPH), flatAmount(0), percentage(0) {}

TaxTile::TaxTile(int index, const std::string& code, const std::string& name, TaxType taxType, int flatAmount, int percentage)
    : ActionTile(index, code, name, TileCategory::DEFAULT), taxType(taxType), flatAmount(flatAmount), percentage(percentage) {}

void TaxTile::onLanded(Player& player, GameContext& gameContext) {
    // GameEngine akan manage user interaction untuk pilihan pajak via command handler
    int amountToPay = calculateTaxAmount(player, 1);
    applyTax(player, gameContext, amountToPay);
}

void TaxTile::applyTax(Player& player, GameContext& gameContext, int amountToPay) const {
    try {
        player -= amountToPay;
        int currentTurn = gameContext.getTurnManager()->getCurrentTurn();
        gameContext.getLogger()->log(currentTurn, player.getUsername(), "PAJAK", 
            "Membayar pajak sebesar M" + std::to_string(amountToPay));
    } catch (const InsufficientFundsException& e) {
        // Masuk ke alur kebangkrutan dengan Bank sebagai kreditur (nullptr)
        gameContext.getBankruptcyHandler()->handleBankruptcy(player, nullptr, amountToPay, gameContext);
    }
}

int TaxTile::calculateTaxAmount(const Player& player, int choice) const {
    // choice 1 = flat, choice 2 = percentage
    if (taxType == TaxType::PPH && choice == 2) {
        return (calculateWealth(player) * percentage) / 100;
    }
    return flatAmount;
}

int TaxTile::calculateWealth(const Player& player) const {
    // Kekayaan = Tunai + Harga Beli Properti + Harga Bangunan
    int total = player.getBalance();
    
    for (auto* property : player.getProperties()) {
        total += property->getBuyPrice();
        
        // Cek jika properti adalah StreetTile untuk menghitung harga bangunan
        StreetTile* street = dynamic_cast<StreetTile*>(property);
        if (street) {
            int level = street->getBuildingLevel(); // 1-4 rumah, 5 hotel
            if (level > 0 && level <= 4) {
                total += level * street->getHouseCost();
            } else if (level == 5) {
                total += (4 * street->getHouseCost()) + street->getHotelCost();
            }
        }
    }
    return total;
}

std::string TaxTile::getDisplayLabel() const { return getCode(); }
TaxType TaxTile::getTaxType() const { return taxType; }
int TaxTile::getFlatAmount() const { return flatAmount; }
int TaxTile::getPercentage() const { return percentage; }