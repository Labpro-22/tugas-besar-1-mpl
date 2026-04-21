#include "models/tiles/UtilityTile.hpp"

UtilityTile::UtilityTile() : PropertyTile() {}

UtilityTile::UtilityTile(
    int index, const std::string& code, const std::string& name, 
    int mortgageValue, const std::map<int, int>& multiplierTable
) : PropertyTile(index, code, name, 0, mortgageValue), multiplierTable(multiplierTable) {}

void UtilityTile::onLanded(Player& player, GameContext& gameContext) {
    if (getStatus() == PropertyStatus::BANK) {
        // Otomatis dimiliki oleh pemain tanpa bayar
        transferTo(player);
        //gameContext.logEvent("UTILITY", player.getName() + " mendapatkan " + getName() + " secara otomatis.");
    } else {
        //gameContext.triggerRentEvent(player, *this);
    }
}

int UtilityTile::calculateRent(int diceTotal, const GameContext& gameContext) {
    if (getStatus() != PropertyStatus::OWNED) return 0;

    // int count = gameContext.getUtilityCount(*getOwner());
    // if (multiplierTable.find(count) != multiplierTable.end()) {
    //     return diceTotal * multiplierTable.at(count);
    // }
    return 0;
}

int UtilityTile::getSellValueToBank() const {
    // Sesuai config, harga lahan Utility adalah 0.
    return 0;
}

std::string UtilityTile::getDisplayLabel() const {
    return "[UTILITY] " + getName();
}